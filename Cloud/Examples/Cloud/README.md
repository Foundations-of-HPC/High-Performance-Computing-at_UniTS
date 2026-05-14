# Terraform on OpenStack - - Lab Exercises

**Course**: HPC and Basic Cloud - University of Trieste  
**Module**: Infrastructure as Code with Terraform  

---

## Overview

This lab introduces Terraform through three progressively complex exercises, all targeting an OpenStack cloud. The first two exercises are adapted from the open-source collection [diodonfrost/terraform-openstack-examples](https://github.com/diodonfrost/terraform-openstack-examples) and simplified for a teaching context. The third exercise deploys a functional HPC cluster with SLURM job scheduling and a shared NFS filesystem.

Each exercise lives in its own self-contained directory. You can work through them in order, or jump directly to any one of them.

```
terraform-lab/
├── README.md                  ← this file
├── singleInstance/            ← Exercise 1: one VM + network
├── multipleInstances/         ← Exercise 2: two VMs + volume
└── hpc-cluster/               ← Exercise 3: SLURM cluster (master + 2 nodes)
```

---

## Prerequisites (all exercises)

**Software**:

```bash
# Terraform >= 1.5 (or OpenTofu >= 1.6)
terraform version

# OpenStack CLI (for looking up resource names)
pip install python-openstackclient
openstack --version

# SSH key pair
ls ~/.ssh/id_rsa.pub
# If missing: ssh-keygen -t ed25519
```

**Credentials**: download the `openrc.sh` file for your project from the OpenStack Horizon dashboard (*Project → API Access → Download OpenStack RC File*), then load it before any Terraform command:

```bash
source ~/openrc.sh
```

**Find your resource names** - you will need these for every exercise:

```bash
source ~/openrc.sh

# Which external network provides floating IPs?
openstack network list --external

# Which images are available?
openstack image list --status active

# Which flavors (VM sizes) are available?
openstack flavor list

# Do you already have a keypair registered?
openstack keypair list
```

---

## Exercise 1 - Single Instance with Network

**Directory**: `singleInstance/`  
**Difficulty**: Beginner  
**Time**: ~20 minutes  
**Concepts**: provider, resource, data source, floating IP, security group

### What it does

Deploys the smallest meaningful cloud infrastructure: one virtual machine on a private network, reachable from the internet via SSH through a floating IP.

```
Internet
    │  SSH (port 22)
    ▼
floating IP  ──►  VM (Ubuntu 22.04)
                  private IP: 192.168.10.x
                  internal network
```

### Resources created

| Resource | Type | Purpose |
|---|---|---|
| `mhpc-network` | `openstack_networking_network_v2` | Private L2 network for the VM |
| `mhpc-subnet` | `openstack_networking_subnet_v2` | Subnet 192.168.10.0/24 with DHCP |
| `mhpc-router` | `openstack_networking_router_v2` | Router connecting private net to external |
| router interface | `openstack_networking_router_interface_v2` | Plugs subnet into router |
| `mhpc-sg` | `openstack_networking_secgroup_v2` | Firewall: allows SSH from anywhere |
| `mhpc-instance` | `openstack_compute_instance_v2` | The virtual machine |
| floating IP | `openstack_networking_floatingip_v2` | Public IP allocated from external pool |
| floating IP association | `openstack_compute_floatingip_associate_v2` | Binds floating IP to the VM |

### Configuration

```bash
cd singleInstance/
cp terraform.tfvars.example terraform.tfvars
```

Edit `terraform.tfvars`:

```hcl
# Adapt these three values to your OpenStack environment
external_network_name = "external"    # from: openstack network list --external
image_name            = "Ubuntu 22.04"# from: openstack image list
flavor_name           = "m1.small"    # from: openstack flavor list
keypair_name          = "my-key"      # from: openstack keypair list
```

### Deploy and connect

```bash
terraform init
terraform plan
terraform apply

# The floating IP appears in the output
ssh ubuntu@<FLOATING_IP>
```

### What to observe and understand

After the VM is up:

```bash
# From inside the VM: verify network connectivity
ip addr show          # see the private IP on eth0
curl -s ifconfig.me   # see the floating IP (the public face)
ping 8.8.8.8          # verify outbound internet through the router

# From your machine: inspect what Terraform created
terraform state list  # all managed resources
terraform state show openstack_compute_instance_v2.instance  # VM details
terraform output      # floating IP and SSH command
```

**Questions to answer**:

1. The VM has one IP on `eth0` (the private IP) but you connect to it via the floating IP. What OpenStack component translates between the two? At which layer of the network stack does this translation happen?
2. The security group allows SSH from `0.0.0.0/0`. What does this mean? How would you restrict access to only your laptop's IP?
3. Run `terraform plan` again without changing anything. What does Terraform report? Why?
4. Change `flavor_name` to a different flavor. Run `terraform plan`. Does Terraform plan to destroy and recreate the VM, or can it resize in place?

### Clean up

```bash
terraform destroy
```

---

## Exercise 2 - Two Instances with a Shared Volume

**Directory**: `multipleInstances/`  
**Difficulty**: Intermediate  
**Time**: ~30 minutes  
**Concepts**: count, block storage, volume attachment, inter-instance networking

### What it does

Deploys two virtual machines on the same private network, with a persistent block storage volume attached to the first instance. The second instance can communicate with the first over the internal network.

```
Internet
    │  SSH (port 22)
    ▼
floating IP  ──►  instance-1 (Ubuntu 22.04)       instance-2 (Ubuntu 22.04)
                  private IP: 192.168.20.x   ←──►  private IP: 192.168.20.y
                  + 10 GB block volume              (no floating IP)
```

### Resources created

Everything from Exercise 1, plus:

| Resource | Type | Purpose |
|---|---|---|
| `instance-2` | `openstack_compute_instance_v2` | Second VM (no floating IP) |
| `data-volume` | `openstack_blockstorage_volume_v3` | 10 GB persistent Cinder volume |
| volume attachment | `openstack_compute_volume_attach_v2` | Attaches the volume to instance-1 |

The floating IP is assigned only to instance-1. Instance-2 is reachable from instance-1 via the private network, but not directly from the internet - this is a common pattern in cloud architectures where only a "bastion" or "jump host" is publicly exposed.

### Configuration

```bash
cd multipleInstances/
cp terraform.tfvars.example terraform.tfvars
```

Edit `terraform.tfvars`:

```hcl
external_network_name = "external"
image_name            = "Ubuntu 22.04"
flavor_name           = "m1.small"
keypair_name          = "my-key"
instance_count        = 2       # controls how many VMs to create
volume_size_gb        = 10      # size of the Cinder block volume
```

### Deploy

```bash
terraform init
terraform plan
terraform apply
```

### Working with the block volume

After SSH into instance-1:

```bash
# The volume appears as a block device, but it needs a filesystem
lsblk
# You should see vdb (or sdb) - the attached Cinder volume

# Create a filesystem on the volume (only needed once)
sudo mkfs.ext4 /dev/vdb

# Mount it
sudo mkdir -p /data
sudo mount /dev/vdb /data
df -h /data

# Write something
echo "Hello from instance-1 at $(date)" | sudo tee /data/test.txt

# Unmount
sudo umount /data
```

### Testing inter-instance connectivity

```bash
# From instance-1, reach instance-2 by private IP
# (get instance-2's IP from: terraform output)
INSTANCE2_IP=$(terraform output -json instance_ips | jq -r '.["instance-2"]')

ping $INSTANCE2_IP
ssh ubuntu@$INSTANCE2_IP   # works because both share the keypair

# From instance-2, ping the external internet
ping 8.8.8.8   # works through NAT on the router
curl -s ifconfig.me  # shows instance-1's floating IP? or instance-2's real IP?
```

### What to observe and understand

**Questions to answer**:

1. `lsblk` on instance-1 shows `vdb`. Where does the name `vdb` come from? What would it be if you attached a second volume?
2. A Cinder block volume is a persistent resource: it survives even if the VM is deleted. Verify this by running `terraform destroy`, then `terraform apply` again. Does the volume retain the data you wrote? What does the Terraform plan show about the volume's lifecycle?
3. Instance-2 has no floating IP. How would you SSH into it from your laptop? Write the exact SSH command using instance-1 as a jump host.
4. The `count` meta-argument controls how many VMs are created. Change `instance_count` to 3 and run `terraform plan`. What exactly does Terraform plan to do? Run `terraform apply`. Does it touch instance-1 and instance-2, or only create instance-3?
5. What happens to the volume if the VM it is attached to is deleted? What happens to the data on it?

### Clean up

```bash
terraform destroy
# Note: the volume is also destroyed (it is managed by Terraform)
# If you want to keep the volume, remove the volume resource from state first:
# terraform state rm openstack_blockstorage_volume_v3.data_volume
```

---

## Exercise 3 - Minimal HPC Cluster with SLURM and NFS

**Directory**: `hpc-cluster/`  
**Difficulty**: Advanced  
**Time**: ~60 minutes (including wait time for cloud-init)  
**Concepts**: templatefile(), cloud-init, NFS, SLURM, munge, count, depends_on, fixed IPs

### What it does

Deploys a complete, functional HPC cluster: one master node running the SLURM controller and an NFS server, plus two compute nodes that mount the shared filesystem and run SLURM worker daemons. After deployment you can submit batch jobs, run MPI programs across both nodes, and observe realistic HPC cluster behaviour.

```
Internet
    │  SSH (floating IP, master only)
    ▼
master  192.168.100.10  ←── slurmctld + NFS server + slurmd
    │
    │  NFS /shared + SLURM + munge
    ├──► node01  192.168.100.11  slurmd + NFS client
    └──► node02  192.168.100.12  slurmd + NFS client
```

### What makes this exercise different

The first two exercises create infrastructure and leave configuration to you. This exercise also automates the entire software configuration using **cloud-init** shell scripts that Terraform generates and passes to each VM at boot time. The scripts are not static - they are **templates** that Terraform fills in with the correct IP addresses, hostnames, and cluster parameters before the VMs ever start.

This is the core IaC pattern for provisioning real systems: infrastructure and configuration are defined together, from a single source of truth.

### Key concepts introduced

**`templatefile()`**: Terraform's function for generating text files with variable substitutions. Used to produce the cloud-init bootstrap scripts for master and compute nodes, with all IPs and SLURM parameters filled in correctly.

**cloud-init `user_data`**: A script passed to the VM at boot, executed once as root. This is how virtually all cloud VMs are configured without manual SSH intervention.

**Fixed Neutron ports**: Unlike the previous exercises where IPs are assigned by DHCP, this cluster uses Neutron ports with explicit fixed IPs. SLURM's configuration file (`slurm.conf`) references compute nodes by IP address, so those addresses must be stable across reboots.

**munge**: The authentication system used by SLURM. All daemons across the cluster must share an identical secret key. The master generates this key and places it on the NFS shared filesystem; compute nodes retrieve it during boot. This staged bootstrap through a shared filesystem is a common pattern for secrets distribution in automated cluster provisioning.

**`depends_on`**: Explicit dependency declaration between resources that do not share data references. Compute VMs depend on the master existing and having its floating IP, but Terraform cannot infer this from the code - it must be stated explicitly.

### Configuration

```bash
cd hpc-cluster/
cp terraform.tfvars.example terraform.tfvars
```

Edit `terraform.tfvars`. Pay particular attention to `compute_cpus` and `compute_mem_mb` - these must exactly reflect the actual hardware of your chosen flavor:

```hcl
external_network_name = "external"
image_name            = "Ubuntu 22.04"
master_flavor         = "m1.medium"
compute_flavor        = "m1.medium"
compute_node_count    = 2
keypair_name          = "my-key"
public_key_path       = "~/.ssh/id_rsa.pub"
compute_cpus          = 2       # must match vCPUs of compute_flavor
compute_mem_mb        = 3500    # must be slightly below RAM of compute_flavor
```

Verify your flavor's actual resources:
```bash
openstack flavor show m1.medium
# vcpus: 2
# ram: 4096    →  use compute_mem_mb = 3500 (leave 500 MB headroom)
```

### Deploy

```bash
terraform init
terraform plan    # read the plan carefully - 15+ resources
terraform apply
```

Terraform completes in ~2 minutes. The VMs then run their cloud-init scripts for another 3–8 minutes. Monitor progress:

```bash
# Follow the master's bootstrap log in real time
ssh ubuntu@<FLOATING_IP> 'sudo tail -f /var/log/cluster_init.log'
```

The cluster is ready when you see:
```
=== [timestamp] Bootstrap master completato ===
```

### Verifying and using the cluster

```bash
ssh ubuntu@<FLOATING_IP>

# Check all nodes are idle
sinfo
# PARTITION AVAIL  NODES  STATE  NODELIST
# compute*     up      2   idle  node[01-02]

# Run a command on both compute nodes
srun -N 2 hostname

# Submit a batch job
cat > /shared/test.sh << 'EOF'
#!/bin/bash
#SBATCH --nodes=2
#SBATCH --ntasks=4
#SBATCH --output=/shared/test_%j.out
echo "Running on: $SLURM_NODELIST"
srun hostname
EOF
sbatch /shared/test.sh
squeue
cat /shared/test_*.out

# MPI job
mpicc -o /shared/hello_mpi /shared/hello_mpi.c  # (see hpc-cluster/README.md)
sbatch /shared/mpi_job.sh
```

### What to observe and understand

**Questions to answer**:

1. Read `/var/log/cluster_init.log` on both the master and node01. How long did node01 wait before NFS became available? What would happen if that wait exceeded 300 seconds?

2. Read `/etc/slurm/slurm.conf` on the master. Identify the lines that were generated by the `%{ for i, ip in node_ips ~}` loop in the template. Now look at `/shared/slurm.conf`. Are they identical? Why does the compute node read from `/shared` rather than have the file placed directly?

3. Run `md5sum /etc/munge/munge.key` on the master, then on node01 and node02. What must be true about these values? What would happen to the cluster if they differed?

4. The `depends_on` in `compute.tf` forces Terraform to create the master before the compute nodes. Remove it (comment out the `depends_on` block), run `terraform destroy`, then `terraform apply`. Does anything break? If so, what is the failure mode and why?

5. Edit `terraform.tfvars` and change `compute_node_count = 4`. Run `terraform plan`. Which resources does Terraform plan to create? Which does it plan to leave unchanged? Run `terraform apply` and verify `sinfo` shows four compute nodes.

6. The master runs both `slurmctld` and `slurmd`. Submit a job that requests 3 nodes (`--nodes=3`) when only 2 compute nodes exist. What does SLURM report? Now run `scontrol show partition compute` - does the partition include the master?

### Full documentation

The `hpc-cluster/` directory contains a detailed README with complete documentation of every file, the template system, the munge key distribution strategy, troubleshooting procedures, and further exercises. Read it alongside the source files.

### Clean up

```bash
terraform destroy
```

---

## Comparison: What Each Exercise Teaches

| Concept | Exercise 1 | Exercise 2 | Exercise 3 |
|---|:---:|:---:|:---:|
| Provider and authentication | ✓ | ✓ | ✓ |
| Virtual network + subnet + router | ✓ | ✓ | ✓ |
| Security groups | ✓ | ✓ | ✓ |
| Floating IP | ✓ | ✓ | ✓ (master only) |
| `count` meta-argument | | ✓ | ✓ |
| Block storage (Cinder volume) | | ✓ | |
| Volume attachment | | ✓ | |
| Jump host / bastion pattern | | ✓ | ✓ |
| Fixed IPs via Neutron ports | | | ✓ |
| `templatefile()` | | | ✓ |
| cloud-init `user_data` | | | ✓ |
| `depends_on` | | | ✓ |
| NFS shared filesystem | | | ✓ |
| SLURM job scheduling | | | ✓ |
| Multi-service orchestration | | | ✓ |

---

## Terraform Workflow Reference

These four commands cover 95% of your daily Terraform use:

```bash
# Download provider plugins (run once per directory)
terraform init

# Show what Terraform will create/change/destroy - never skips this
terraform plan

# Create or update infrastructure to match the .tf files
terraform apply

# Destroy all managed infrastructure
terraform destroy
```

Additional commands that are useful for learning:

```bash
# List all resources currently managed by Terraform
terraform state list

# Show full details of one resource
terraform state show <resource_address>

# Print output values
terraform output
terraform output -json   # machine-readable

# Validate syntax without contacting OpenStack
terraform validate

# Reformat .tf files to canonical style
terraform fmt
```

---

## Common Mistakes

**Not sourcing openrc.sh before running Terraform**:
```bash
# Wrong: Terraform cannot authenticate
terraform apply

# Correct:
source ~/openrc.sh && terraform apply
```

**Wrong image or flavor name**: names are case-sensitive and must match exactly what `openstack image list` shows. `"ubuntu 22.04"` ≠ `"Ubuntu 22.04"`.

**`compute_cpus` or `compute_mem_mb` mismatch in Exercise 3**: if you tell SLURM a node has 8 CPUs but the VM only has 2, SLURM will overcommit resources and jobs will fail with confusing errors. Always verify with `openstack flavor show <name>`.

**Forgetting `terraform destroy`**: cloud resources cost quota even when idle. Always destroy when you finish a session.

**Running `terraform apply` from the wrong directory**: each exercise has its own state file. Always `cd` into the exercise directory first.

---

## Credits

Exercises 1 and 2 are adapted from [diodonfrost/terraform-openstack-examples](https://github.com/diodonfrost/terraform-openstack-examples) (LGPL-3.0), modified for compatibility with Terraform provider `openstack ~> 2.0` and simplified for teaching purposes.

Exercise 3 is an original exercise developed for this course.
