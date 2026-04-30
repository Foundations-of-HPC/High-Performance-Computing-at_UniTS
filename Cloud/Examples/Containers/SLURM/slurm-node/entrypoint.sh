#!/bin/bash
set -e

# Start the Munge authentication service
echo "Starting Munge..."
service munge start

# Ensure proper permissions and ownership
chown -R munge: /var/lib/munge /var/log/munge /run/munge

# Start SSH Daemon
echo "Starting SSH..."
service ssh start

mkdir -p /var/spool/slurm/ctld
chown -R slurm.slurm /var/spool/slurm/
mkdir /var/log/slurm/
chown slurm.slurm /var/log/slurm
# Start Slurm daemon
echo "Starting Slurmd..."
/usr/sbin/slurmd -D