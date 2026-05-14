#!/bin/bash
# ==============================================================================
# compute_init.sh.tpl — Cloud-init per i nodi di calcolo
#
# Variabili iniettate da Terraform:
#   node_name, node_ip, master_ip, shared_path, all_node_ips
# ==============================================================================

set -euo pipefail
exec > >(tee /var/log/cluster_init.log) 2>&1

echo "=== [$(date '+%Y-%m-%d %H:%M:%S')] Bootstrap ${node_name} avviato ==="

# ── 1. Hostname e /etc/hosts ──────────────────────────────────────────────────
hostnamectl set-hostname ${node_name}

cat >> /etc/hosts << 'HOSTS'
${master_ip}  master
%{ for i, ip in all_node_ips ~}
${ip}  node0${i + 1}
%{ endfor ~}
HOSTS

# ── 2. Pacchetti ──────────────────────────────────────────────────────────────
export DEBIAN_FRONTEND=noninteractive
apt-get update -q
apt-get install -yq \
    slurmd \
    munge \
    nfs-common \
    openmpi-bin \
    libopenmpi-dev \
    build-essential \
    htop vim

# ── 3. Attendi NFS dal master ─────────────────────────────────────────────────
# Il master potrebbe ancora stare eseguendo il suo cloud-init.
# Polling con timeout generoso.
MAX_WAIT=300
WAITED=0
echo "Attendo NFS su ${master_ip}..."
until showmount -e ${master_ip} 2>/dev/null | grep -q "${shared_path}"; do
    if [ "$WAITED" -ge "$MAX_WAIT" ]; then
        echo "ERRORE: NFS non disponibile dopo $${MAX_WAIT}s. Uscita."
        exit 1
    fi
    sleep 5
    WAITED=$((WAITED + 5))
    echo "  ... $${WAITED}s / $${MAX_WAIT}s"
done
echo "NFS disponibile dopo $${WAITED}s"

# ── 4. Monta /shared ──────────────────────────────────────────────────────────
mkdir -p ${shared_path}
mount -t nfs ${master_ip}:${shared_path} ${shared_path}

# Rendi persistente al riavvio
echo "${master_ip}:${shared_path}  ${shared_path}  nfs  defaults,_netdev,nofail  0  0" \
    >> /etc/fstab

echo "Montato ${master_ip}:${shared_path} → ${shared_path}"

# ── 5. Munge key dal master ───────────────────────────────────────────────────
WAITED=0
echo "Attendo munge.key in ${shared_path}..."
until [ -f "${shared_path}/munge.key" ]; do
    if [ "$WAITED" -ge "$MAX_WAIT" ]; then
        echo "ERRORE: munge.key non trovata dopo $${MAX_WAIT}s."
        exit 1
    fi
    sleep 3
    WAITED=$((WAITED + 3))
done

install -m 0400 -o munge -g munge \
    ${shared_path}/munge.key /etc/munge/munge.key

systemctl enable --now munge
echo "Munge: avviato"

# ── 6. slurm.conf dal master ──────────────────────────────────────────────────
WAITED=0
until [ -f "${shared_path}/slurm.conf" ]; do
    if [ "$WAITED" -ge "$MAX_WAIT" ]; then
        echo "ERRORE: slurm.conf non trovata dopo $${MAX_WAIT}s."
        exit 1
    fi
    sleep 3
    WAITED=$((WAITED + 3))
done

mkdir -p /etc/slurm
cp ${shared_path}/slurm.conf  /etc/slurm/slurm.conf
cp ${shared_path}/cgroup.conf /etc/slurm/cgroup.conf

# ── 7. Directory SLURM e avvio slurmd ────────────────────────────────────────
mkdir -p /var/log/slurm /var/lib/slurm/slurmd
chown -R slurm:slurm /var/log/slurm /var/lib/slurm

systemctl enable --now slurmd

# ── 8. Utente hpcuser (stesso UID del master) ─────────────────────────────────
if [ -f "${shared_path}/setup_user.sh" ]; then
    bash ${shared_path}/setup_user.sh
fi

echo ""
echo "=== [$(date '+%Y-%m-%d %H:%M:%S')] Bootstrap ${node_name} completato ==="
echo "slurmd : $(systemctl is-active slurmd)"
echo "munge  : $(systemctl is-active munge)"
