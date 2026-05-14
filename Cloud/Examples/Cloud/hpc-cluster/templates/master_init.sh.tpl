#!/bin/bash
# ==============================================================================
# master_init.sh.tpl — Cloud-init per il master node HPC
#
# Variabili iniettate da Terraform templatefile():
#   cluster_name, master_ip, shared_path, node_count,
#   compute_cpus, compute_mem_mb, node_ips (lista)
# ==============================================================================

set -euo pipefail
exec > >(tee /var/log/cluster_init.log) 2>&1

echo "=== [$(date '+%Y-%m-%d %H:%M:%S')] Bootstrap master avviato ==="

# ── 1. Hostname e /etc/hosts ──────────────────────────────────────────────────
hostnamectl set-hostname master

# Aggiungi tutti i nodi a /etc/hosts (essenziale per MPI e SLURM)
cat >> /etc/hosts << 'HOSTS'
${master_ip}  master
%{ for i, ip in node_ips ~}
${ip}  node0${i + 1}
%{ endfor ~}
HOSTS

# ── 2. Pacchetti ──────────────────────────────────────────────────────────────
export DEBIAN_FRONTEND=noninteractive
apt-get update -q
apt-get install -yq \
    nfs-kernel-server \
    nfs-common \
    slurm-wlm \
    munge \
    openmpi-bin \
    libopenmpi-dev \
    build-essential \
    htop vim tree

# ── 3. Filesystem condiviso NFS ───────────────────────────────────────────────
mkdir -p ${shared_path}
chmod 1777 ${shared_path}

# Esporta /shared a tutta la subnet interna
echo "${shared_path} 192.168.0.0/16(rw,sync,no_root_squash,no_subtree_check)" \
    >> /etc/exports

systemctl enable --now nfs-kernel-server
exportfs -ra
echo "NFS: ${shared_path} esportata"

# ── 4. Munge (autenticazione SLURM) ──────────────────────────────────────────
# Genera la chiave munge sul master, poi la copia in /shared
# I compute node la leggeranno da lì

# Crea una chiave casuale da 1024 byte
dd if=/dev/urandom bs=1 count=1024 2>/dev/null \
    | base64 -w 0 \
    | head -c 1024 \
    > /etc/munge/munge.key

chmod 0400 /etc/munge/munge.key
chown munge:munge /etc/munge/munge.key

# Copia in /shared perché i compute la leggano
install -m 0400 /etc/munge/munge.key ${shared_path}/munge.key

systemctl enable --now munge
echo "Munge: avviato, chiave in ${shared_path}/munge.key"

# ── 5. slurm.conf ─────────────────────────────────────────────────────────────
mkdir -p /etc/slurm

cat > /etc/slurm/slurm.conf << SLURMCONF
# slurm.conf — Cluster ${cluster_name} — generato da Terraform cloud-init

ClusterName=${cluster_name}
SlurmctldHost=master(${master_ip})

AuthType=auth/munge
CryptoType=crypto/munge

# Tracking dei processi via cgroup (standard moderno)
ProctrackType=proctrack/cgroup
TaskPlugin=task/affinity,task/cgroup

SchedulerType=sched/backfill
SelectType=select/cons_tres
SelectTypeParameters=CR_Core_Memory

# Log
SlurmctldLogFile=/var/log/slurm/slurmctld.log
SlurmdLogFile=/var/log/slurm/slurmd.log
SlurmctldPidFile=/run/slurmctld.pid
SlurmdPidFile=/run/slurmd.pid

# State
StateSaveLocation=/var/lib/slurm/slurmctld
SlurmdSpoolDir=/var/lib/slurm/slurmd

# Porte default SLURM
SlurmctldPort=6817
SlurmdPort=6818

# Nessun accounting (minimale)
JobCompType=jobcomp/none
AccountingStorageType=accounting_storage/none

# Cloud-friendly: i nodi tornano idle automaticamente dopo reboot
ReturnToService=2

# Timeout (rilassati per cloud/didattica)
SlurmctldTimeout=120
SlurmdTimeout=300
InactiveLimit=0
MinJobAge=300
KillWait=30

# ── Nodi ──────────────────────────────────────────────────────────────────────
%{ for i, ip in node_ips ~}
NodeName=node0${i + 1} NodeAddr=${ip} CPUs=${compute_cpus} RealMemory=${compute_mem_mb} State=UNKNOWN
%{ endfor ~}

# ── Partizione ────────────────────────────────────────────────────────────────
PartitionName=compute \
    Nodes=node[01-0${node_count}] \
    Default=YES \
    MaxTime=INFINITE \
    State=UP
SLURMCONF

# cgroup.conf (richiesto da ProctrackType=proctrack/cgroup)
cat > /etc/slurm/cgroup.conf << 'EOF'
CgroupPlugin=cgroup/v2
EOF

# Directory SLURM
mkdir -p /var/log/slurm /var/lib/slurm/slurmctld /var/lib/slurm/slurmd
chown -R slurm:slurm /var/log/slurm /var/lib/slurm

# Copia config in /shared per i compute node
cp /etc/slurm/slurm.conf  ${shared_path}/slurm.conf
cp /etc/slurm/cgroup.conf ${shared_path}/cgroup.conf

# ── 6. Avvia SLURM sul master ─────────────────────────────────────────────────
systemctl enable --now slurmctld slurmd

# ── 7. Crea un utente hpcuser su tutti i nodi (via /shared/setup_user.sh) ─────
# I compute node eseguiranno questo script dopo aver montato /shared
cat > ${shared_path}/setup_user.sh << 'USERSCRIPT'
#!/bin/bash
# Crea l'utente hpcuser con UID fisso (identico su tutti i nodi)
if ! id hpcuser &>/dev/null; then
    useradd -m -u 2000 -s /bin/bash hpcuser
    echo "hpcuser:hpcuser" | chpasswd
    usermod -aG sudo hpcuser
fi
USERSCRIPT
chmod +x ${shared_path}/setup_user.sh
bash ${shared_path}/setup_user.sh   # esegui anche sul master

echo ""
echo "=== [$(date '+%Y-%m-%d %H:%M:%S')] Bootstrap master completato ==="
echo "Cluster : ${cluster_name}"
echo "Master  : master (${master_ip})"
%{ for i, ip in node_ips ~}
echo "Compute : node0${i + 1} (${ip})"
%{ endfor ~}
echo ""
echo "Verifica cluster: sinfo"
echo "Job di test:      srun -N ${node_count} hostname"
