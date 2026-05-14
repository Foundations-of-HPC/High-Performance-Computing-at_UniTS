################################################################################
# main.tf — Provider OpenStack + rete interna + security groups + keypair
#
# Autenticazione: NON mettere credenziali qui.
# Carica prima il tuo openrc.sh:
#   source ~/openrc.sh
# Terraform legge automaticamente le variabili d'ambiente OS_*.
################################################################################

terraform {
  required_version = ">= 1.5.0"

  required_providers {
    openstack = {
      source  = "terraform-provider-openstack/openstack"
      version = "~> 2.0"
    }
  }
}

# Il provider OpenStack legge le variabili OS_* dal file openrc.sh.
# Non serve specificare credenziali esplicitamente.
provider "openstack" {}

# ── Data source: rete external (per i floating IP) ────────────────────────────
data "openstack_networking_network_v2" "external" {
  name = var.external_network_name
}

# ── Data source: immagine ─────────────────────────────────────────────────────
data "openstack_images_image_v2" "ubuntu" {
  name        = var.image_name
  most_recent = true
}

# ── Keypair SSH ───────────────────────────────────────────────────────────────
# Importa la chiave pubblica locale nel progetto OpenStack.
# Se la keypair esiste già: commenta questo blocco e usa solo keypair_name.
resource "openstack_compute_keypair_v2" "cluster" {
  name       = var.keypair_name
  public_key = file(var.public_key_path)
}

# ── Rete interna del cluster ──────────────────────────────────────────────────
resource "openstack_networking_network_v2" "cluster" {
  name           = "${var.cluster_name}-net"
  admin_state_up = true
}

resource "openstack_networking_subnet_v2" "cluster" {
  name            = "${var.cluster_name}-subnet"
  network_id      = openstack_networking_network_v2.cluster.id
  cidr            = var.internal_subnet_cidr
  ip_version      = 4
  dns_nameservers = var.dns_nameservers
}

# ── Router: collega la rete interna alla rete externa ─────────────────────────
resource "openstack_networking_router_v2" "cluster" {
  name                = "${var.cluster_name}-router"
  admin_state_up      = true
  external_network_id = data.openstack_networking_network_v2.external.id
}

resource "openstack_networking_router_interface_v2" "cluster" {
  router_id = openstack_networking_router_v2.cluster.id
  subnet_id = openstack_networking_subnet_v2.cluster.id
}

# ── Security group: master ─────────────────────────────────────────────────────
resource "openstack_networking_secgroup_v2" "master" {
  name        = "${var.cluster_name}-master-sg"
  description = "Master node: SSH da internet + traffico cluster interno"
}

# SSH da fuori
resource "openstack_networking_secgroup_rule_v2" "master_ssh" {
  direction         = "ingress"
  ethertype         = "IPv4"
  protocol          = "tcp"
  port_range_min    = 22
  port_range_max    = 22
  remote_ip_prefix  = "0.0.0.0/0"
  security_group_id = openstack_networking_secgroup_v2.master.id
}

# Tutto il traffico dalla rete interna (NFS, SLURM, munge, ICMP)
resource "openstack_networking_secgroup_rule_v2" "master_internal" {
  direction         = "ingress"
  ethertype         = "IPv4"
  remote_ip_prefix  = var.internal_subnet_cidr
  security_group_id = openstack_networking_secgroup_v2.master.id
}

# ── Security group: compute nodes ─────────────────────────────────────────────
resource "openstack_networking_secgroup_v2" "compute" {
  name        = "${var.cluster_name}-compute-sg"
  description = "Compute nodes: solo traffico dalla rete interna"
}

resource "openstack_networking_secgroup_rule_v2" "compute_internal" {
  direction         = "ingress"
  ethertype         = "IPv4"
  remote_ip_prefix  = var.internal_subnet_cidr
  security_group_id = openstack_networking_secgroup_v2.compute.id
}
