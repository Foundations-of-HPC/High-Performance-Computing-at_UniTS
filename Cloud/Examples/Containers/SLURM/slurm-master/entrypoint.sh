#!/bin/bash
set -e

# Start the Munge authentication service
echo "Starting Munge..."
service munge start
#service munge stop
#chown -R munge: /var/log/munge/munged.log
#/usr/sbin/munged --force

# Ensure proper permissions and ownership
chown -R munge: /var/lib/munge /var/log/munge /run/munge
mkdir -p /var/spool/slurm/ctld
chown -R slurm.slurm /var/spool/slurm/
mkdir /var/log/slurm/
chown slurm.slurm /var/log/slurm

# Start SSH Daemon
echo "Starting SSH..."
/usr/sbin/sshd

# Start Slurm controller
echo "Starting Slurm controller..."
/usr/sbin/slurmctld -D