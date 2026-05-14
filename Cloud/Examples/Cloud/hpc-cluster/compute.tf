################################################################################
# compute.tf — Nodi di calcolo: slurmd + NFS client
#
# count = var.compute_node_count (default: 2)
# IP privati fissi: .11, .12, ... (via cidrhost())
# Nessun floating IP: i compute node sono raggiungibili solo dal master via SSH
################################################################################

# Un port Neutron per ciascun compute node (IP fisso)
resource "openstack_networking_port_v2" "compute" {
  count              = var.compute_node_count
  name               = "${var.cluster_name}-node0${count.index + 1}-port"
  network_id         = openstack_networking_network_v2.cluster.id
  admin_state_up     = true
  security_group_ids = [openstack_networking_secgroup_v2.compute.id]

  fixed_ip {
    subnet_id  = openstack_networking_subnet_v2.cluster.id
    ip_address = cidrhost(var.internal_subnet_cidr, var.compute_ip_base + count.index)
  }

  depends_on = [
    openstack_networking_subnet_v2.cluster,
    openstack_networking_secgroup_v2.compute,
  ]
}

# Istanze compute node
resource "openstack_compute_instance_v2" "compute" {
  count       = var.compute_node_count
  name        = "${var.cluster_name}-node0${count.index + 1}"
  image_id    = data.openstack_images_image_v2.ubuntu.id
  flavor_name = var.compute_flavor
  key_pair    = openstack_compute_keypair_v2.cluster.name

  network {
    port = openstack_networking_port_v2.compute[count.index].id
  }

  user_data = templatefile("${path.module}/templates/compute_init.sh.tpl", {
    node_name    = "node0${count.index + 1}"
    node_ip      = cidrhost(var.internal_subnet_cidr, var.compute_ip_base + count.index)
    master_ip    = var.master_ip
    shared_path  = var.shared_fs_path
    # Tutti gli IP dei compute (per popolare /etc/hosts completo)
    all_node_ips = [
      for i in range(var.compute_node_count) :
      cidrhost(var.internal_subnet_cidr, var.compute_ip_base + i)
    ]
  })

  # Aspetta che il master esista prima di creare i compute node.
  # Il master deve avere NFS attivo affinché il polling nel template funzioni.
  depends_on = [
    openstack_compute_instance_v2.master,
    openstack_compute_floatingip_associate_v2.master,
  ]
}
