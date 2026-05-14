################################################################################
# variables.tf — Minimal HPC cluster (SLURM + NFS) su OpenStack / ADA Cloud
################################################################################

variable "cluster_name" {
  description = "Prefisso per nomi di tutte le risorse"
  type        = string
  default     = "mhpc"
}

# ── Rete ───────────────────────────────────────────────────────────────────────
variable "external_network_name" {
  description = "Nome della rete external/provider da cui allocare i floating IP"
  type        = string
  default     = "external"   # su ADA Cloud CINECA: tipicamente "external" o "public"
}

variable "internal_subnet_cidr" {
  description = "CIDR della rete interna del cluster"
  type        = string
  default     = "192.168.100.0/24"
}

variable "dns_nameservers" {
  description = "DNS da configurare nella subnet interna"
  type        = list(string)
  default     = ["8.8.8.8", "8.8.4.4"]
}

# IP statici assegnati ai nodi via Neutron port (dentro la subnet interna)
variable "master_ip" {
  description = "IP privato fisso del master node"
  type        = string
  default     = "192.168.100.10"
}

variable "compute_ip_base" {
  description = "Terzo ottetto dei compute node: .11, .12, ..."
  type        = number
  default     = 11   # node01=.11, node02=.12
}

# ── Immagine e flavor ──────────────────────────────────────────────────────────
variable "image_name" {
  description = "Nome dell'immagine da usare (deve esistere nel tuo progetto OpenStack)"
  type        = string
  default     = "Ubuntu 22.04"   # adatta al nome esatto in ADA Cloud
}

variable "master_flavor" {
  description = "Flavor per il master node"
  type        = string
  default     = "m1.medium"   # 2 vCPU, 4 GB RAM — adatta al catalogo ADA Cloud
}

variable "compute_flavor" {
  description = "Flavor per i compute node"
  type        = string
  default     = "m1.medium"
}

variable "compute_node_count" {
  description = "Numero di nodi di calcolo"
  type        = number
  default     = 2
}

# ── SSH ────────────────────────────────────────────────────────────────────────
variable "keypair_name" {
  description = "Nome della keypair OpenStack da usare (deve già esistere nel progetto)"
  type        = string
  default     = "mhpc-key"
}

variable "public_key_path" {
  description = "Path della chiave pubblica locale da importare come keypair"
  type        = string
  default     = "~/.ssh/id_rsa.pub"
}

# ── Cluster ────────────────────────────────────────────────────────────────────
variable "shared_fs_path" {
  description = "Mount point del filesystem condiviso NFS su tutti i nodi"
  type        = string
  default     = "/shared"
}

variable "compute_cpus" {
  description = "CPU per nodo di calcolo (deve corrispondere al flavor scelto)"
  type        = number
  default     = 2
}

variable "compute_mem_mb" {
  description = "RAM per nodo di calcolo in MB (lascia ~500MB di headroom)"
  type        = number
  default     = 3500
}
