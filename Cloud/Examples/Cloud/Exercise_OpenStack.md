# Exercise - OpenStack Fundamentals: Hands-on with CINECA ADA Cloud

| Field | Value |
|-------|-------|
| **Course** | Cloud Computing, ADSAI, Trieste |
| **Academic Year** | 2025/2026 |
| **Type** | Practical / Hands-on |
| **Infrastructure** | CINECA ADA Cloud (OpenStack Wallaby) |

---

## Overview

This exercise provides hands-on experience with CINECA ADA Cloud, an OpenStack Wallaby deployment available to MHPC students. You will perform the complete lifecycle of a research computing environment: authentication, network configuration, instance provisioning, storage attachment, and security hardening. All tasks mirror real workflows you would use in production research computing.


---



## Part 1: Environment Setup and Authentication 

### Task 1.1: CLI Installation and Authentication 

Install the OpenStack Python client in a virtual environment and authenticate against CINECA ADA Cloud:

```bash
# Create and activate a virtual environment
python3 -m venv ~/openstack-env
source ~/openstack-env/bin/activate

# Install the OpenStack client (Wallaby-compatible version)
pip install python-openstackclient==5.8.0

# Source your Application Credentials (downloaded from Horizon)
source ~/your-app-cred-openrc.sh

# Verify authentication
openstack token issue
openstack project list
```

**Deliverable**: Paste the output of `openstack token issue` and `openstack project list` into your submission. Redact the token value itself.

### Task 1.2: Resource Inventory 

Explore what resources are available in your ADA Cloud project:

```bash
# List available images
openstack image list

# List available flavors
openstack flavor list

# Check your current quota
openstack quota show
openstack limits show --absolute
```

**Analysis question**: Based on `openstack limits show --absolute`, answer:
- How many instances can you currently create?
- How much total RAM is available to your project?
- How many floating IPs are you allowed?

**Deliverable**: Table showing the 5 most resource-relevant quota limits for your project.

### Task 1.3: Horizon Exploration

Log in to the Horizon dashboard at https://adacloud.hpc.cineca.it and explore the following:

- Navigate to **Compute → Instances** (should be empty initially)
- Navigate to **Network → Network Topology** (observe any pre-existing resources)
- Navigate to **Identity → Application Credentials** (observe your credential)

**Deliverable**: Annotated screenshot of the Network Topology view, or a written description of any pre-existing resources in your project.

---

## Part 2: Network Infrastructure 

### Task 2.1: Create Private Network Stack 

Create a complete private network environment for your analysis workload:

```bash
# Create a private network
openstack network create \
  --description "HPC analysis private network" \
  hpc-analysis-net

# Create a subnet
openstack subnet create \
  --network hpc-analysis-net \
  --subnet-range 192.168.42.0/24 \
  --gateway 192.168.42.1 \
  --dns-nameserver 8.8.8.8 \
  hpc-analysis-subnet

# Create a router
openstack router create hpc-analysis-router

# Attach router to external network (use the correct external network name for ADA)
# Check available networks: openstack network list --external
openstack router set \
  --external-gateway <external-network-name> \
  hpc-analysis-router

# Connect router to your subnet
openstack router add subnet hpc-analysis-router hpc-analysis-subnet

# Verify the topology
openstack network list
openstack subnet list
openstack router list
openstack router show hpc-analysis-router
```

**Deliverable**: Output of `openstack router show hpc-analysis-router`. Highlight the `external_gateway_info` field and explain what it means.

### Task 2.2: Security Group Configuration 

Create a purpose-specific security group for your analysis VM:

```bash
# Create the security group
openstack security group create \
  --description "Security group for HPC analysis VM" \
  hpc-analysis-sg

# Allow SSH from your institution's IP range (replace with actual SISSA CIDR)
openstack security group rule create \
  --protocol tcp \
  --dst-port 22 \
  --remote-ip 0.0.0.0/0 \   # Replace with your institution's CIDR in production
  hpc-analysis-sg

# Allow ICMP (ping) for diagnostics
openstack security group rule create \
  --protocol icmp \
  --remote-ip 0.0.0.0/0 \
  hpc-analysis-sg

# Allow Jupyter notebook access (port 8888) from your IP
openstack security group rule create \
  --protocol tcp \
  --dst-port 8888 \
  --remote-ip 0.0.0.0/0 \
  hpc-analysis-sg

# List all rules to verify
openstack security group rule list hpc-analysis-sg
```

**Analysis question**: The instructions above use `--remote-ip 0.0.0.0/0` for SSH. Explain why this is a security risk and what the correct practice would be. 

**Deliverable**: Table of security group rules (copy from `openstack security group rule list` output) + written answer to the analysis question.

---

## Part 3: Instance Provisioning and Management 

### Task 3.1: Launch an Analysis VM 

Launch a virtual machine using the network and security group created in Part 2:

```bash
# Register your SSH key
openstack keypair create \
  --public-key ~/.ssh/id_rsa.pub \
  hpc-keypair

# Select an appropriate image (Ubuntu 22.04 or similar)
openstack image list | grep -i ubuntu

# Launch the instance
openstack server create \
  --image "Ubuntu 24.04" \     
  --flavor m1.medium \
  --network hpc-analysis-net \
  --key-name hpc-keypair \
  --security-group hpc-analysis-sg \
  --user-data - << 'EOF'
#!/bin/bash
apt-get update -qq
apt-get install -y python3-pip python3-numpy python3-scipy ncbi-hpc+
pip3 install jupyter
echo "Analysis VM ready" > /tmp/init-complete
EOF
  hpc-analysis-vm

# Wait for ACTIVE state (poll every 10s)
watch -n 10 'openstack server show hpc-analysis-vm | grep status'
```

### Task 3.2: Assign a Floating IP 

```bash
# Allocate a floating IP
openstack floating ip create <external-network-name>

# Show allocated floating IPs
openstack floating ip list

# Associate with your instance
openstack server add floating ip hpc-analysis-vm <allocated-floating-ip>

# Verify association
openstack server show hpc-analysis-vm | grep -A 10 addresses
```

**Deliverable**: Output of `openstack server show hpc-analysis-vm`, highlighting the assigned floating IP.

### Task 3.3: Connect and Verify 

```bash
# SSH into your instance
ssh -i ~/.ssh/id_rsa ubuntu@<floating-ip>

# Verify cloud-init completed
cat /tmp/init-complete
# Expected: "Analysis VM ready"

# Verify Python packages
python3 -c "import numpy, scipy; print('NumPy', numpy.__version__, 'SciPy', scipy.__version__)"

# Exit the VM
exit
```

**Deliverable**: Terminal screenshot or text capture showing successful SSH login,  and Python package versions.

### Task 3.4: Instance Lifecycle Operations

Demonstrate instance lifecycle control:

```bash
# Show current instance state
openstack server show hpc-analysis-vm | grep status

# Stop the instance (graceful OS shutdown)
openstack server stop hpc-analysis-vm

# Wait for SHUTOFF state
openstack server show hpc-analysis-vm | grep status

# Restart the instance
openstack server start hpc-analysis-vm

# View the console log (useful for debugging boot issues)
openstack console log show hpc-analysis-vm | tail -20
```

**Analysis question**: What happens to the floating IP association when you stop and restart the instance? Does the floating IP remain associated? Verify your answer empirically.

**Deliverable**: Commands and output demonstrating stop/start cycle. Answer to the analysis question.

---

## Part 4: Block Storage 

### Task 4.1: Create and Attach a Volume

Create a persistent volume for storing hpc database files:

```bash
# Create a 50 GB volume for genomic data
openstack volume create \
  --size 50 \
  --description "HPC database and analysis data" \
  hpc-data-vol

# Verify volume creation
openstack volume list
openstack volume show hpc-data-vol

# Attach to the running VM
openstack server add volume hpc-analysis-vm hpc-data-vol

# Verify attachment
openstack volume show hpc-data-vol | grep -A 5 attachments
```

### Task 4.2: Format, Mount, and Use the Volume 

```bash
# SSH into the instance
ssh -i ~/.ssh/id_rsa ubuntu@<floating-ip>

# Identify the newly attached block device
lsblk
# Expected: /dev/vdb (the new volume appears as a virtio block device)

# Format the volume (ext4 filesystem)
sudo mkfs.ext4 /dev/vdb

# Create mount point and mount
sudo mkdir -p /data/hpc-db
sudo mount /dev/vdb /data/hpc-db

# Verify mount
df -h /data/hpc-db

# Add to /etc/fstab for persistence across reboots
echo "/dev/vdb /data/hpc-db ext4 defaults 0 2" | sudo tee -a /etc/fstab

# Write test data (simulate downloading a hpc database)
sudo bash -c 'dd if=/dev/urandom bs=1M count=100 | base64 > /data/hpc-db/test-genome.fa'
ls -lh /data/hpc-db/

# Verify the device is the Cinder volume, not ephemeral disk
sudo lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE
```

**Analysis question**: What is the fundamental difference between the root disk of your instance (`/dev/vda`) and the Cinder volume (`/dev/vdb`)? What happens to each if you run `openstack server delete hpc-analysis-vm`?

**Deliverable**: Output of `df -h /data/hpc-db` and `lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE`. Written answer to the analysis question.

---

## Part 5: Security Analysis

### Task 5.1: Security Group Audit 

Perform a security audit of your deployment:

```bash
# List all security group rules applied to your instance
openstack server show hpc-analysis-vm | grep security_groups

# List rules for each security group
openstack security group rule list hpc-analysis-sg
openstack security group rule list default

# Check the 'default' security group rules
openstack security group show default
```

Answer the following questions:

1. What traffic does the `default` security group permit by default in your project?
2. Is the `default` security group attached to your instance? If so, does this create any security concerns given your `hpc-analysis-sg` rules?
3. Classify each rule in `hpc-analysis-sg` as: (a) required for functionality, (b) useful for operations, or (c) potentially risky. Justify each classification.

**Deliverable**: Written answers to questions 1-3, supported by the security group rule output.

### Task 5.2: Network Path Analysis 

Trace the network path from your workstation to your VM:

```bash
# From your workstation (not the VM):
# Test connectivity
ping -c 4 <floating-ip>

# Trace the path (requires traceroute installed)
traceroute <floating-ip>

# From inside the VM:
ssh ubuntu@<floating-ip>

# Verify internal network configuration
ip addr show
ip route show
cat /etc/resolv.conf

# Test that workers on the same subnet can resolve DNS
nslookup google.com

# Test that the instance can reach the internet
curl -s --max-time 5 https://ipinfo.io/ip
```

**Analysis question**: Draw a network diagram showing the path from your workstation to the VM, including: your workstation, the CINECA campus network, the ADA Cloud external network, the virtual router, the tenant subnet, and the VM. Label IP addresses at each hop where known.

**Deliverable**: Network diagram (ASCII art, hand-drawn and photographed, or any tool) + output of `ip addr show` and `ip route show` from inside the VM.

---

## Cleanup (Required)

**Important**: Always clean up ADA Cloud resources when you are finished. Quotas are shared across all students.

```bash
# Remove floating IP association and release it
openstack server remove floating ip hpc-analysis-vm <floating-ip>
openstack floating ip delete <floating-ip>

# Delete the instance (but NOT the volume - data persists)
openstack server delete hpc-analysis-vm

# Verify volume is now available (not attached)
openstack volume show hpc-data-vol

# Optionally: delete the volume if done (data is lost)
openstack volume delete hpc-data-vol

# Remove network infrastructure
openstack router remove subnet hpc-analysis-router hpc-analysis-subnet
openstack router unset --external-gateway hpc-analysis-router
openstack router delete hpc-analysis-router
openstack subnet delete hpc-analysis-subnet
openstack network delete hpc-analysis-net

# Remove security group
openstack security group delete hpc-analysis-sg

# Remove keypair
openstack keypair delete hpc-keypair

# Verify all resources are removed
openstack server list
openstack volume list
openstack floating ip list
openstack network list
```

**Deliverable**: Output of the final verification commands (showing empty lists or only pre-existing resources).

---

## Submission Checklist

- [ ] Part 1: Token issue output, quota table, Horizon screenshot
- [ ] Part 2: Router show output, security group rule table + analysis
- [ ] Part 3: Server show output with floating IP, terminal screenshot with hpc/Python versions, lifecycle analysis
- [ ] Part 4: df/lsblk output, volume durability analysis
- [ ] Part 5: Security group audit answers, network diagram, ip/route output
- [ ] Cleanup: Final empty resource listings

---

## Instructor Solutions

### Part 2.2 Security Analysis (Expected Answer)

Using `0.0.0.0/0` for SSH exposes port 22 to the entire internet. Automated scanning bots will attempt brute-force logins within minutes of exposure. The correct practice is to restrict SSH to your institution's IP range. At SISSA/ICTP, the appropriate CIDR is the SISSA network block (e.g., `140.105.0.0/16` - students should verify the exact range with the IT office). A more secure alternative is to use a bastion host pattern: expose only the bastion with strict IP restrictions, and access all other VMs through it.

### Part 4.2 Volume Durability Analysis (Expected Answer)

The root disk (`/dev/vda`) is **ephemeral storage** tied to the Nova instance. It exists only as long as the instance exists. When `openstack server delete` is executed, the root disk is destroyed with it. The data on `/dev/vda` is irretrievably lost.

The Cinder volume (`/dev/vdb`) is **persistent block storage** independent of any instance. When `openstack server delete` is executed, the volume is *detached* automatically but **not deleted**. The data on the volume survives and can be attached to a new instance. This is why critical research data must always be stored on Cinder volumes (or Swift), never on the ephemeral root disk.

---

*Cloud Computing -  Course: ADSAI HPC e Cloud, University of Trieste · 2025/2026*
