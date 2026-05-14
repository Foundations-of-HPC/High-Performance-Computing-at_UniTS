################################################################################
# master.tf — Nodo master: slurmctld + NFS server + login
#
# IP privato fisso (via Neutron port): 192.168.100.10
# Floating IP: allocato dal pool external → associato al master
################################################################################

# Port Neutron con IP fisso per il master
resource "openstack_networking_port_v2" "master" {
  name               = "${var.cluster_name}-master-port"
  network_id         = openstack_networking_network_v2.cluster.id
  admin_state_up     = true
  # Metti i security group sul port, non sull'istanza (best practice OpenStack)
  security_group_ids = [openstack_networking_secgroup_v2.master.id]

  fixed_ip {
    subnet_id  = openstack_networking_subnet_v2.cluster.id
    ip_address = var.master_ip
  }

  # Il port dipende dalla subnet e dal sg
  depends_on = [
    openstack_networking_subnet_v2.cluster,
    openstack_networking_secgroup_v2.master,
  ]
}

# Istanza master
resource "openstack_compute_instance_v2" "master" {
  name      = "${var.cluster_name}-master"
  image_id  = data.openstack_images_image_v2.ubuntu.id
  flavor_name = var.master_flavor
  key_pair  = openstack_compute_keypair_v2.cluster.name

  # Aggancia il port Neutron (con IP fisso e security group)
  network {
    port = openstack_networking_port_v2.master.id
  }

  # Cloud-init: templatefile() sostituisce le variabili nel template bash
  user_data = templatefile("${path.module}/templates/master_init.sh.tpl", {
    cluster_name   = var.cluster_name
    master_ip      = var.master_ip
    shared_path    = var.shared_fs_path
    node_count     = var.compute_node_count
    compute_cpus   = var.compute_cpus
    compute_mem_mb = var.compute_mem_mb
    # Lista IP dei compute node: .11, .12, ...
    node_ips = [
      for i in range(var.compute_node_count) :
      cidrhost(var.internal_subnet_cidr, var.compute_ip_base + i)
    ]
  })

  depends_on = [
    openstack_networking_router_interface_v2.cluster,
    openstack_networking_port_v2.master,
  ]
}

# Floating IP dal pool external
resource "openstack_networking_floatingip_v2" "master" {
  pool = var.external_network_name
}

# Associa il floating IP al port del master (metodo raccomandato)
resource "openstack_compute_floatingip_associate_v2" "master" {
  floating_ip = openstack_networking_floatingip_v2.master.address
  instance_id = openstack_compute_instance_v2.master.id
  fixed_ip    = openstack_networking_port_v2.master.fixed_ip[0].ip_address
}
