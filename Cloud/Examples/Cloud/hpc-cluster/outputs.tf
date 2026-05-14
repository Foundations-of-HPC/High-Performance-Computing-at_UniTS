################################################################################
# outputs.tf — Informazioni utili dopo terraform apply
################################################################################

output "master_floating_ip" {
  description = "IP pubblico del master node — usa questo per SSH"
  value       = openstack_networking_floatingip_v2.master.address
}

output "master_private_ip" {
  description = "IP privato del master nella rete interna del cluster"
  value       = var.master_ip
}

output "compute_node_ips" {
  description = "IP privati dei nodi di calcolo"
  value = {
    for i in range(var.compute_node_count) :
    "node0${i + 1}" => cidrhost(var.internal_subnet_cidr, var.compute_ip_base + i)
  }
}

output "ssh_master" {
  description = "Comando SSH per accedere al master"
  value       = "ssh ubuntu@${openstack_networking_floatingip_v2.master.address}"
}

output "check_cloud_init" {
  description = "Controlla il log cloud-init sul master"
  value       = "ssh ubuntu@${openstack_networking_floatingip_v2.master.address} 'sudo tail -30 /var/log/cluster_init.log'"
}

output "check_cluster" {
  description = "Verifica che il cluster SLURM sia in piedi"
  value       = "ssh ubuntu@${openstack_networking_floatingip_v2.master.address} 'sinfo && echo OK'"
}

output "ssh_to_compute_via_master" {
  description = "I compute node non hanno floating IP: accedi via master come jump host"
  value = {
    for i in range(var.compute_node_count) :
    "node0${i + 1}" => "ssh -J ubuntu@${openstack_networking_floatingip_v2.master.address} ubuntu@node0${i + 1}"
  }
}
