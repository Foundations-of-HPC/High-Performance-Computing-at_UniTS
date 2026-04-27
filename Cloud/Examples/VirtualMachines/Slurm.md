# Virtualization Tutorial

In this tutorial, we will learn how to build a cluster of Linux machines on our local environment using Virtualbox. 
Each machine will have two NICs one internal and one to connect to WAN .
We will connect to them from our host windows machine via SSH.

This configuration is useful to try out a clustered application which requires multiple Linux machines like kubernetes or an HPC cluster on your local environment.
The primary goal will be to test the guest virtual machine performances using standard benckmaks as HPL, STREAM or iozone and compare with the host performances.

Then we will installa a slurm based cluster to test parallel applications

## GOALs
In this tutorial, we are going to create an HPC cluster of four Linux virtual machines based on slurm.
Here I will describe a simple configuration of the slurm management tool for launching jobs in a really simplistic Virtual cluster. I will assume the following configuration: a main node (master) and 3 compute nodes (node01, node02, node03). I also assume there is ping access between the nodes and some sort of mechanism for you to know the IP of each node at all times (most basic should be a local NAT with static IPs)

## Prerequisite

* VirtualBOX installed in your linux/windows/Apple
* ubuntu server 24.04 LTS image to install
* SSH client to connect

## Create virtual machines on Virtualbox
We create one template that we will use then to deploy the cluster and to make some performance tests and comparisons

Create the template virtual machine which we will name "template" with 1vCPUs, 1GB of RAM and 25 GB hard disk. 

You can use Ubuntu 24.04 LTS server (https://ubuntu.com/download/server)

Make sure to set up the network as follows:

 * Attach the downloaded Ubuntu ISO to the "ISO Image".
 * Type: is Lunux
 * Version Ubuntu 24.04 LTS
When you start the virtual machines for the first time you are prompted to instal and setup Ubuntu. 
Follow through with the installation until you get to the “Network Commections”. As the VM network protocol is NAT,  the virtual machine will be assinged to an automatic IP (internal) and it will be able to access internet for software upgrades. 

The VM is now accessing the network to download the software and updates for the LTS. 

When you are prompted for the "Guided storage configuration" panel keep the default installation method: use an entire disk. 

When you are prompted for the Profile setup, you will be requested to define a server name (template) and super user (e.g. user01) and his administrative password.


Also, enable open ssh server in the software selection prompt.

Follow the installation and then shutdown the VM.

Inspect the VM and in particular the Network, you will  find only one adapter attached to NAT. If you look at the advanced tab you will find the Adapter Type (Intel) and the MAC address.

Start the newly created machine and make sure it is running. 

Login and update the software:

```
$ sudo apt update
...

$ sudo apt upgrade
```


When your VM is all set up, log in to the VM and test the network by pinging any public site. e.g `ping google.com`. If all works well, this should be successful.

You can check the DHCP-assigned IP address by entering the following command:

```shell
hostname -I
```

You will get an output similar to this:
```
10.0.2.15
```

This is the default IP address assigned by your network DHCP. Note that this IP address is dynamic and can change or worst still, get assigned to another machine. But for now, you can connect to this IP from your host machine via SSH.

Now install some useful additional packages:

```
$ sudo apt install net-tools openssh-server
```

### Install gcc and OpenMPI

```
$ sudo apt install gcc-13 g++-13 make openmpi-bin openmpi-common  libopenmpi-dev
```

Configure gcc-14 as default:
```
$ sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100
$ sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100
$ sudo update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-13 100
```
Test the installation:

```
$ gcc --version 
gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
Copyright (C) 2023 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```
and MPI

```
$ mpicc --version
gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
Copyright (C) 2023 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```
### Install additional software

Lets start installing munge authentication tool using the system package manager, for cluster01 and cluster03:

```
$ sudo apt-get install -y libmunge-dev libmunge2 munge
```

Install slurm

```
$ sudo apt-get install -y slurmd slurmctld  slurm-client

$ sudo apt install -y nfs-common
```

Install and disable dnsmasq (necessary for later configuration)

```
$ sudo apt install dnsmasq -y
$ sudo systemctl disable dnsmasq
```



### Reboot and Clone

If everything is ok, we can proceed cloning this template. We will create 3 clones (you can create more than 3 
according to the amount of RAM and cores available in your laptop).




You must shutdown the node to clone it, using VirtualBox interface (select VM and right click) create 3 new VMs. 

```
$ sudo shutdown -h now
```


Right click on the name of the VM and clone it. The first clone will be the login/master node the other twos will be computing nodes.

## Create host-only network
Use the Network tool in Oracle VirtualBox Manager to create and manage the networks used by Oracle VirtualBox VMs to communicate each other, the host machine, and external resources.

The network types available are:
 * **Host-only**. A network for VMs to communicate internally on this machine, but not with external networks.
 * **NAT**. The default network used by VirtualBox, suitable for most communication between VMs and external networks.
 * **Cloud**. A network used to connect to Oracle Cloud Infrastructure.

Host-only networking  can be used to create a network containing the host and a set of virtual machines, without the need for the host's physical network interface. Instead, a virtual network interface, similar to a loopback interface, is created on the host, providing connectivity among virtual machines and the host. A DHCP internal server is  providing IPs to the VMs. In Host-only networking VMs can communicate but there is not Internet access. 
In VirtualBox Application select Network and then  Create Host-only network. Call the new network *CloudBasicNet* and Keep the Mask, set the lower bound to ```192.168.56.2``` and the upper bound to ```192.168.56.15```. This will assign ip address ```192.168.56.2``` to the host while we can keep ```192.168.56.1``` to the masternode. 

## Configure the cluster

Once the 2 machines has been cloned we can bootstrap the login/master node and configure it.
Add a second network adapter: enable "Adapter 2" "Attached to" internal network and name it *CloudBasicNet*.

### Login/master node

Bootstrap the VM and configure the secondary network adapter with a static IP. 

In the example below the interface is enp0s8, to find your own one:

```
$ ip link show
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN mode DEFAULT group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: enp0s8: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP mode DEFAULT group default qlen 1000
    link/ether 08:00:27:18:9e:04 brd ff:ff:ff:ff:ff:ff
3: enp0s9: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP mode DEFAULT group default qlen 1000
    link/ether 08:00:27:0a:24:c2 brd ff:ff:ff:ff:ff:ff
```

You are interested to link 2 and 3. Link 2 is the NAT device, Link 3 is the internal network device.
You are interested in Link 3. Remeber the name of this link (enp0s9).

Now we configure the adapter. To do this we will edit the netplan file:

```
$ sudo vim /etc/netplan/00-installer-config.yaml

# This is the network config written by 'subiquity'
network:
  ethernets:
    enp0s8:
      dhcp4: true
    enp0s9:
     dhcp4: no
     addresses: [192.168.56.1/24]
  version: 2
```

and apply the configuration

```
$ sudo netplan apply
```
We change the hostname:
```
$ sudo vim /etc/hostname

master
```

#### Access the login/master node from host
To enable ssh from host to guest VM you need to 

```
ssh user01@192.158.56.1
```

but you will have to enter the password. 
If you want a passwordless access you need to generate a ssh key or use an ssh key if you already have it.

If you don’t have public/private key pair already, run ssh-keygen and agree to all defaults. 
This will create id_rsa (private key) and id_rsa.pub (public key) in ~/.ssh directory.

Copy host public key to your VM:

```
scp  ~/.ssh/id_rsa.pub user01@192.158.56.1:~
```

Connect to the VM and add host public key to ~/.ssh/authorized_keys:

```
ssh  user01@192.158.56.1
mkdir ~/.ssh
chmod 700 ~/.ssh
cat ~/id_rsa.pub >> ~/.ssh/authorized_keys
chmod 644 ~/.ssh/authorized_keys
exit
```
#### Configure hostanames

Edit the hosts file to assign names to the cluster that should include names for each node as follows:

```
$ sudo vim /etc/hosts

127.0.0.1 localhost
192.168.56.1 master

192.168.56.3  node03
192.168.56.4  node04
192.168.56.5  node05
192.168.56.6  node06
192.168.56.7  node07
192.168.56.8  node08
192.168.56.9  node09
192.168.56.10 node10
192.168.56.11 node11
192.168.56.12 node12
192.168.56.13 node13
192.168.56.14 node14
192.168.56.15 node15



# The following lines are desirable for IPv6 capable hosts
::1     ip6-localhost ip6-loopback
fe00::0 ip6-localnet
ff00::0 ip6-mcastprefix
ff02::1 ip6-allnodes
ff02::2 ip6-allrouters

```

#### Configure DNS for internal name lookup
Then we install a DNSMASQ server to dynamically assign hostname to the other nodes on the internal interface and create a cluster [1].



```
$ sudo systemctl stop systemd-resolved
$ sudo systemctl stop dnsmasq
```
Edit /etc/default/dnsmasq and define the following:

```
# If the resolvconf package is installed, dnsmasq will use its output
# rather than the contents of /etc/resolv.conf to find upstream
# nameservers. Uncommenting this line inhibits this behaviour.
# Note that including a "resolv-file=<filename>" line in
# /etc/dnsmasq.conf is not enough to override resolvconf if it is
# installed: the line below must be uncommented.
IGNORE_RESOLVCONF=yes

# If the resolvconf package is installed, dnsmasq will tell resolvconf
# to use dnsmasq under 127.0.0.1 as the system's default resolver.
# Uncommenting this line inhibits this behaviour.
DNSMASQ_EXCEPT="lo"
```

Both of these need to be defined even though the resolvconf application is not installed by default on Ubuntu 24.04 Server. This is due to /sbin/resolvconf being a symlink to /bin/resolvectl.

To find and configuration file for Dnsmasq, navigate to /etc/dnsmasq.conf. Edit the file by modifying it with your desired configs. Below is minimal configurations for it to run and support minimum operations.

```
$ sudo  vim /etc/dnsmasq.conf

no-resolv
no-poll
server=127.0.0.1#5353
listen-address=127.0.0.1,::1,192.168.56.1
bind-interfaces
```

Enable and start the dnsmasq service

```
$ sudo systemctl enable dnsmasq
$ sudo systemctl start dnsmasq
$ systemctl status dnsmasq
```

Check that it's listening on port 53 at 127.0.0.1:
```
$ sudo netstat -tlpn
Active Internet connections (only servers)
Proto Recv-Q Send-Q Local Address           Foreign Address         State       PID/Program name    
tcp        0      0 127.0.0.1:53            0.0.0.0:*               LISTEN      5752/dnsmasq    
tcp        0      0 192.168.56.1:53         0.0.0.0:*               LISTEN      4635/dnsmasq          
tcp6       0      0 :::22                   :::*  
```

Remove /etc/resolv.conf

```
sudo rm /etc/resolv.conf
```

Create a new /etc/resolv.conf with the *Dnsmasq* nameserver:

```
echo "nameservers 127.0.0.1" | sudo tee /etc/resolv.conf
```

Edit */etc/systemd/resolved.conf* and define the following:

```
DNSStubListener=no
DNSStubListenerExtra=127.0.0.1:5353
Cache=no
```
Start systemd-resolved service and check it's status:
```
$ sudo systemctl start systemd-resolved
$ systemctl status systemd-resolved
user01@master:~$ systemctl status systemd-resolved
● systemd-resolved.service - Network Name Resolution
     Loaded: loaded (/usr/lib/systemd/system/systemd-resolved.service; enabled; preset: enabled)
     Active: active (running) since Thu 2026-04-23 16:22:36 UTC; 10s ago
       Docs: man:systemd-resolved.service(8)
             man:org.freedesktop.resolve1(5)
             https://www.freedesktop.org/wiki/Software/systemd/writing-network-configuration-managers
             https://www.freedesktop.org/wiki/Software/systemd/writing-resolver-clients
   Main PID: 1626 (systemd-resolve)
     Status: "Processing requests..."
      Tasks: 1 (limit: 2206)
     Memory: 2.5M (peak: 2.7M)
        CPU: 48ms
     CGroup: /system.slice/systemd-resolved.service
             └─1626 /usr/lib/systemd/systemd-resolved

Apr 23 16:22:36 master systemd[1]: Starting systemd-resolved.service - Network Name Resolution...
Apr 23 16:22:36 master systemd-resolved[1626]: Positive Trust Anchors:
Apr 23 16:22:36 master systemd-resolved[1626]: . IN DS 20326 8 2 e06d44b80b8f1d39a95c0b0d7c65d08458e880409bbc6834571042>
Apr 23 16:22:36 master systemd-resolved[1626]: Negative trust anchors: home.arpa 10.in-addr.arpa 16.172.in-addr.arpa 17>
Apr 23 16:22:36 master systemd-resolved[1626]: Using system hostname 'master'.
Apr 23 16:22:36 master systemd[1]: Started systemd-resolved.service - Network Name Resolution.

```
Check that it's listening on port 5353 at 127.0.0.1:

```
$ sudo netstat -tlpn
Active Internet connections (only servers)
Proto Recv-Q Send-Q Local Address           Foreign Address         State       PID/Program name    
tcp        0      0 0.0.0.0:22              0.0.0.0:*               LISTEN      1/init              
tcp        0      0 127.0.0.1:5353          0.0.0.0:*               LISTEN      1626/systemd-resolv 
tcp        0      0 127.0.0.1:53            0.0.0.0:*               LISTEN      1548/dnsmasq        
tcp6       0      0 :::22                   :::*                    LISTEN      1/init              
```

Run a query. If configured correctly, the query should resolve.

```
$ dig -4 google.com

; <<>> DiG 9.18.39-0ubuntu0.24.04.3-Ubuntu <<>> -4 node02
;; global options: +cmd
;; Got answer:
;; ->>HEADER<<- opcode: QUERY, status: SERVFAIL, id: 20204
;; flags: qr aa rd ra; QUERY: 1, ANSWER: 0, AUTHORITY: 0, ADDITIONAL: 1

;; OPT PSEUDOSECTION:
; EDNS: version: 0, flags:; udp: 1232
;; QUESTION SECTION:
;node02.				IN	A

;; Query time: 0 msec
;; SERVER: 127.0.0.1#53(127.0.0.1) (UDP)
;; WHEN: Thu Apr 23 16:24:45 UTC 2026
;; MSG SIZE  rcvd: 35
```

And also the local nodes:

```
$ dig -4 node04

; <<>> DiG 9.18.39-0ubuntu0.24.04.3-Ubuntu <<>> -4 node04
;; global options: +cmd
;; Got answer:
;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 53974
;; flags: qr aa rd ra; QUERY: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 1

;; OPT PSEUDOSECTION:
; EDNS: version: 0, flags:; udp: 1232
;; QUESTION SECTION:
;node04.				IN	A

;; ANSWER SECTION:
node04.			0	IN	A	192.168.56.4

;; Query time: 0 msec
;; SERVER: 127.0.0.1#53(127.0.0.1) (UDP)
;; WHEN: Thu Apr 23 16:24:47 UTC 2026
;; MSG SIZE  rcvd: 51

```
#### A distributed filesystem

To build a cluster we aldo need a distributed filesystem accessible from all nodes. 
We use NFS.

```
$ sudo apt install nfs-kernel-server
$ sudo mkdir /shared
$ sudo chmod 777 /shared
```

Modify the NFS config file:

```
$ sudo vim /etc/exports

/shared/  192.168.56.0/255.255.255.0(rw,sync,no_root_squash,no_subtree_check)

```
Restart the NFS server

```
$ sudo systemctl enable nfs-kernel-server
$ sudo systemctl restart nfs-kernel-server
```
prepare some directories to share:
```
$ sudo mkdir /shared/data /shared/home
$ sudo chmod 777  /shared/data /shared/home
```

### Computing Node
Now we can bootstrap the first computing node and configure it. We can call this new VM *node01*.

Before boostrapping the node we must configure the netowork connection. modigy the first  network adapter that is now in NAT mode:  "Attached to" host-only network, Name  *CloudBasicNet*.

Bootstrap the node. 

Empty the /etc/hostname file
```
$ sudo vim /etc/hostname

```
Configure netowrk card
```shell
$ sudo vim /etc/netplan/50-cloud-init.yaml

# This is the network config written by 'subiquity'
network:
  ethernets:
    enp0s8:
      dhcp4: true
      dhcp-identifier: mac
```

Set the proper dns server (assigned with dhcp):

```
$ sudo unlink   /etc/resolv.conf

$ sudo vim /etc/resolv.conf

nameserver 192.168.56.1
search .
```

Test the configuration

```
$ host node05
node05 has address 192.168.56.5
```
#### Hostaname assigment
As slurmd requires a valid hostname and we want to nodes set their hostname on the basic of the ip address we must create a scripr that runs at bootstrab before slurm and after the ip assignement. 

##### The hostname-setting script

Create the file `/usr/local/sbin/set-hostname-from-ip.sh` with the following
content and remember to costomize to your ethernet card name (ipp add show):

```bash
#!/bin/bash
set -e

IFACE="enp0s8"
MAX_WAIT=30
WAITED=0

# Attendi che l'interfaccia abbia un IP
while [ $WAITED -lt $MAX_WAIT ]; do
    IP=$(ip -4 -o addr show dev "$IFACE" 2>/dev/null | awk '{print $4}' | cut -d'/' -f1 | head -n1)
    if [ -n "$IP" ]; then
        break
    fi
    sleep 1
    WAITED=$((WAITED+1))
done

if [ -z "$IP" ]; then
    echo "ERRORE: nessun IP trovato su $IFACE dopo ${MAX_WAIT}s" >&2
    exit 1
fi

#
NEW_HOSTNAME=$(getent hosts "$IP" | awk '{print $2}' | cut -d. -f1)

if [ -z "$NEW_HOSTNAME" ]; then
    echo "ERRORE: impossibile derivare hostname da IP $IP" >&2
    exit 1
fi

echo "Imposto hostname a $NEW_HOSTNAME (IP $IFACE: $IP)"
hostnamectl set-hostname "$NEW_HOSTNAME"

# Aggiorna /etc/hosts per evitare warning di sudo e per slurmd
if grep -q "^127\.0\.1\.1" /etc/hosts; then
    sed -i "s/^127\.0\.1\.1.*/127.0.1.1\t${NEW_HOSTNAME}/" /etc/hosts
else
    echo -e "127.0.1.1\t${NEW_HOSTNAME}" >> /etc/hosts
fi

exit 0
```

Make it executable and owned by root:

```bash
sudo chown root:root /usr/local/sbin/set-hostname-from-ip.sh
sudo chmod 755 /usr/local/sbin/set-hostname-from-ip.sh
```


Create the file `/etc/systemd/system/set-hostname-from-ip.service` with the
following content:

```ini
[Unit]
Description=Set hostname based on IP of en08
Wants=network-online.target
After=network-online.target systemd-hostnamed.service
Before=slurmd.service
DefaultDependencies=yes

[Service]
Type=oneshot
ExecStart=/usr/local/sbin/set-hostname-from-ip.sh
RemainAfterExit=yes
TimeoutStartSec=60

[Install]
WantedBy=multi-user.target
RequiredBy=slurmd.service
```

The relevant directives are:

- `After=network-online.target` ensures the network stack is up and the
  interface has had a chance to acquire an address before the script runs.
- `Before=slurmd.service` orders this unit before Slurm's node daemon.
- `RequiredBy=slurmd.service` (in the `[Install]` section) creates a hard
  dependency: if hostname configuration fails, `slurmd` is not started.
- `Type=oneshot` combined with `RemainAfterExit=yes` is the correct pattern
  for a unit that performs a single configuration step and then exits.


Reload systemd and enable the unit:

```bash
sudo systemctl daemon-reload
sudo systemctl enable set-hostname-from-ip.service
```

Because of the `[Install]` section, `enable` creates two symlinks:

- one in `/etc/systemd/system/multi-user.target.wants/`, so the service runs
  on every boot;
- one in `/etc/systemd/system/slurmd.service.requires/`, so `slurmd` waits
  for it and refuses to start if it fails.


You can test the configuration without rebooting:

```bash
# Run the script manually
sudo /usr/local/sbin/set-hostname-from-ip.sh

# Start and inspect the service
sudo systemctl start set-hostname-from-ip.service
sudo systemctl status set-hostname-from-ip.service

# Confirm slurmd's dependency chain
systemctl list-dependencies slurmd.service | grep -i hostname
```

After a reboot, check the journal to confirm correct ordering and timing:

```bash
journalctl -u set-hostname-from-ip.service -b
journalctl -u slurmd.service -b
hostnamectl
```


#### Configure the shared filesystem
```
$ sudo mkdir /shared
```

Mount the shared directory adn test it
```
$ sudo mount 192.168.56.1:/shared  /shared

$ df -h
Filesystem            Size  Used Avail Use% Mounted on
tmpfs                 196M 1016K  195M   1% /run
efivarfs              256K  6.5K  250K   3% /sys/firmware/efi/efivars
/dev/sda2              24G  2.5G   20G  11% /
tmpfs                 978M     0  978M   0% /dev/shm
tmpfs                 5.0M     0  5.0M   0% /run/lock
/dev/sda1             1.1G  6.4M  1.1G   1% /boot/efi
tmpfs                 196M   12K  196M   1% /run/user/1000
192.168.56.1:/shared   24G  2.5G   20G  11% /shared
```
Check that you have R/W access:
```
$ touch /shared/pippo
```
If everything will be ok you will see the "pippo" file in all the nodes.
Unmount the mount pont:
```
$ sudo umount /shared/
```
Edit */etc/fstab* file and add at the end the following line:
```
192.168.56.1:/shared    /shared    nfs    defaults    0 0 

```

then reload ths system

```
$ sudo systemctl daemon-reload
$ sudo mount -a
$ mount

sysfs on /sys type sysfs (rw,nosuid,nodev,noexec,relatime)
proc on /proc type proc (rw,nosuid,nodev,noexec,relatime)
udev on /dev type devtmpfs (rw,nosuid,relatime,size=941424k,nr_inodes=235356,mode=755,inode64)
devpts on /dev/pts type devpts (rw,nosuid,noexec,relatime,gid=5,mode=620,ptmxmode=000)
tmpfs on /run type tmpfs (rw,nosuid,nodev,noexec,relatime,size=200008k,mode=755,inode64)
efivarfs on /sys/firmware/efi/efivars type efivarfs (rw,nosuid,nodev,noexec,relatime)
/dev/sda2 on / type ext4 (rw,relatime)
securityfs on /sys/kernel/security type securityfs (rw,nosuid,nodev,noexec,relatime)
tmpfs on /dev/shm type tmpfs (rw,nosuid,nodev,inode64)
tmpfs on /run/lock type tmpfs (rw,nosuid,nodev,noexec,relatime,size=5120k,inode64)
cgroup2 on /sys/fs/cgroup type cgroup2 (rw,nosuid,nodev,noexec,relatime,nsdelegate,memory_recursiveprot)
pstore on /sys/fs/pstore type pstore (rw,nosuid,nodev,noexec,relatime)
bpf on /sys/fs/bpf type bpf (rw,nosuid,nodev,noexec,relatime,mode=700)
systemd-1 on /proc/sys/fs/binfmt_misc type autofs (rw,relatime,fd=32,pgrp=1,timeout=0,minproto=5,maxproto=5,direct,pipe_ino=5329)
mqueue on /dev/mqueue type mqueue (rw,nosuid,nodev,noexec,relatime)
tracefs on /sys/kernel/tracing type tracefs (rw,nosuid,nodev,noexec,relatime)
hugetlbfs on /dev/hugepages type hugetlbfs (rw,nosuid,nodev,relatime,pagesize=2M)
debugfs on /sys/kernel/debug type debugfs (rw,nosuid,nodev,noexec,relatime)
fusectl on /sys/fs/fuse/connections type fusectl (rw,nosuid,nodev,noexec,relatime)
configfs on /sys/kernel/config type configfs (rw,nosuid,nodev,noexec,relatime)
/dev/sda1 on /boot/efi type vfat (rw,relatime,fmask=0022,dmask=0022,codepage=437,iocharset=iso8859-1,shortname=mixed,errors=remount-ro)
binfmt_misc on /proc/sys/fs/binfmt_misc type binfmt_misc (rw,nosuid,nodev,noexec,relatime)
sunrpc on /run/rpc_pipefs type rpc_pipefs (rw,relatime)
tmpfs on /run/user/1000 type tmpfs (rw,nosuid,nodev,relatime,size=200008k,nr_inodes=50002,mode=700,uid=1000,gid=1000,inode64)
192.168.56.1:/shared on /shared type nfs4 (rw,relatime,vers=4.2,rsize=262144,wsize=262144,namlen=255,hard,proto=tcp,timeo=600,retrans=2,sec=sys,clientaddr=192.168.56.12,local_lock=none,addr=192.168.56.1)
```
**Remember: you must have a master node active before bootstrapping and shuttingdown the nodes**

## Install a SLURM based cluster

Slurm management tool work on a set of nodes, one of which is considered the master node, and has the slurmctld daemon running; all other compute nodes have the slurmd daemon. 

All communications are authenticated via the munge service and all nodes need to share the same authentication key. Slurm by default holds a journal of activities in a directory configured in the slurm.conf file, however a Database management system can be set. All in all what we will try to do is:

 * Install munge in all nodes and configure the same authentication key in each of them
 * Install gcc, openmpi and configure them
 * Configure the slurmctld service in the master node
 * Configure the slurmd service in the compute nodes
 * Create a basic file structure for storing jobs and jobs result that is equal in all the nodes of the cluster
 * Manipulate the state of the nodes, and learn to resume them if they are down
 * Run some simple jobs as test
 * Set up MPI task on the cluster


### Configure MUNGE

Lets start installing munge authentication tool using the system package manager, for master and node.

munge requires that we generate a key file for testing authentication, for this we use the dd utility, with the fast pseudo-random device /dev/urandom. At master node do:
```
$ sudo sh -c 'dd if=/dev/urandom bs=1 count=1024 > /etc/munge/munge.key'
$ sudo chown munge:munge /etc/munge/munge.key
$ sudo chmod 400 /etc/munge/munge.key
```
Edit the default configuration file and uncomment the OPTION line:
```
$ cat /etc/default/munge 
# MUNGE configuration

# Pass additional command-line options to munged.
OPTIONS="--key-file=/etc/munge/munge.key --num-threads=2"
```

Copy the key on node02 with scp and chech also on computing node the file permissions and user.

```
scp /etc/munge/munge.key user01@nodeXX:/etc/munge/munge.key
```
and on computing node:

```
$ chown munge:munge /etc/munge/munge.key
$ chmod 400 /etc/munge/munge.key
```
Also edit the default configuration file and uncomment the OPTION line:
```
$ cat /etc/default/munge 
# MUNGE configuration

# Pass additional command-line options to munged.
OPTIONS="--key-file=/etc/munge/munge.key --num-threads=2"
```

Restart munge in both master and node

```
$ sudo systemctl restart munge
$  systemctl status munge
● munge.service - MUNGE authentication service
     Loaded: loaded (/usr/lib/systemd/system/munge.service; enabled; preset: enabled)
     Active: active (running) since Sun 2026-04-26 14:15:02 UTC; 4s ago
       Docs: man:munged(8)
    Process: 5086 ExecStart=/usr/sbin/munged $OPTIONS (code=exited, status=0/SUCCESS)
   Main PID: 5090 (munged)
      Tasks: 4 (limit: 2206)
     Memory: 624.0K (peak: 1.4M)
        CPU: 3ms
     CGroup: /system.slice/munge.service
             └─5090 /usr/sbin/munged --key-file=/etc/munge/munge.key --num-threads=2

Apr 26 14:15:02 master systemd[1]: Starting munge.service - MUNGE authentication service...
Apr 26 14:15:02 master systemd[1]: Started munge.service - MUNGE authentication service.
```


Test communication with, locally and remotely with these commands respectively:

```
$ munge -n | unmunge
$ munge -n | ssh nodeXX unmunge
```

### Configure Slurm
On master node 
```
$ sudo apt-get install -y  slurmctld
```

Copy the slurm configuration file of GIT repository to '/etc/slurm' directory of in master and computing node.

```bash
ClusterName=virtual
SlurmctldHost=master
ProctrackType=proctrack/linuxproc
ReturnToService=2
SlurmctldPidFile=/run/slurmctld.pid
SlurmdPidFile=/run/slurmd.pid
SlurmdSpoolDir=/var/lib/slurm/slurmd
StateSaveLocation=/var/lib/slurm/slurmctld
SlurmUser=slurm
TaskPlugin=task/none
SchedulerType=sched/backfill
SelectType=select/cons_tres
SelectTypeParameters=CR_Core_Memory
AccountingStorageType=accounting_storage/none
JobCompType=jobcomp/none
JobAcctGatherType=jobacct_gather/none
SlurmctldDebug=info
SlurmctldLogFile=/var/log/slurm/slurmctld.log
SlurmdDebug=info
SlurmdLogFile=/var/log/slurm/slurmd.log
NodeName=node[03-15] NodeAddr=192.168.56.[3-15]  CPUs=1 RealMemory=1953

# PartitionName ################################################################
#
# Name by which the partition may be referenced (e.g. "Interactive").  This
# name can be specified by users when submitting jobs. If the PartitionName is
# "DEFAULT", the values specified with that record will apply to subsequent
# partition specifications unless explicitly set to other values in that
# partition record or replaced with a different set of default values. Each
# line where PartitionName is "DEFAULT" will replace or add to previous default
# values and not a reinitialize the default values.

PartitionName=debug Nodes=ALL Default=YES MaxTime=INFINITE State=UP
```

Notice that it is important to configure nodes with the proper amount of CPUs and RAM (in MB)
to chek the amount of RAM use the:

```shell
$ free -m
               total        used        free      shared  buff/cache   available
Mem:            1953         225        1554           1         252        1728
Swap:              0           0           0
```



On master

```
$ sudo systemctl enable slurmctld
$ sudo systemctl start slurmctld
```

On node02
```
$ sudo systemctl disable slurmctld
$ sudo systemctl stop slurmctld

$ sudo systemctl enable slurmd
$ sudo systemctl start slurmd
```

Now you can test the envoroment:

```
$ sinfo
PARTITION AVAIL  TIMELIMIT  NODES  STATE NODELIST
debug*       up   infinite     12   unk* node[03-11,13-15]
debug*       up   infinite      1   idle node12

```

Test a job:

```
$ srun hostname
node02
```

### Clone the node

Shutdown node02 and clone it to node03 and then start the two VMs.

Afther startup you sould find

```
$ sinfo
PARTITION AVAIL  TIMELIMIT  NODES  STATE NODELIST
debug*       up   infinite      2   idle node[02-03]
debug*       up   infinite      1   unk* node04
```


## Testing VMM enviroment



### Test slurm cluster

 * Run a simple MPI program on the cluster
 * Run an interactive job
 * Use the OSU ping pong benchmark to test the VM interconnect.
 
Install OSU MPI benchmarks: download the latest tarball from http://mvapich.cse.ohio-state.edu/benchmarks/.

```
$ cd /shared
$ wget https://mvapich.cse.ohio-state.edu/download/mvapich/osu-micro-benchmarks-7.5.2.tar.gz
$ tar zxvf osu-micro-benchmarks-7.5.2.tar.gz

$ cd osu-micro-benchmarks-7.5.2/


$ ./configure CC=/usr/bin/mpicc CXX=/usr/bin/mpicxx --prefix=/shared/OSU/
$ make
$ make install
```

#### Run benchmarks

Create a slurm file

```shell
#!/bin/bash
#SBATCH --job-name=MPI-OMP
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=1
#SBATCH -p  debug 
#SBATCH -t 0-12:00 # time (D-HH:MM)
#SBATCH -o slurm.%N.%j.out # STDOUT
#SBATCH -e slurm.%N.%j.err # STDERR

echo $SLURM_NODELIST
mpirun osu_latency
```

## References

[1] Configure DNSMASQ https://computingforgeeks.com/install-and-configure-dnsmasq-on-ubuntu/?expand_article=1

[2] Configure NFS Mounts https://www.howtoforge.com/how-to-install-nfs-server-and-client-on-ubuntu-22-04/

[3] Configure network with netplan https://linuxconfig.org/netplan-network-configuration-tutorial-for-beginners

[4] SLURM Quick Start Administrator Guide https://slurm.schedmd.com/quickstart_admin.html

[5] Simple SLURM configuration on Debian systems: https://gist.github.com/asmateus/301b0cb86700cbe74c269b27f2ecfbef

[6] HPC Challege https://hpcchallenge.org/hpcc/

[7] IO Zone Benchmarks https://www.iozone.org/
