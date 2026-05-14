# Params file for variables

#### GLANCE
variable "image" {
  type    = string
  default = "Ubuntu-24.04"
}

#### NEUTRON
variable "external_network" {
  type    = string
  default = "public"
}

# UUID of external gateway
variable "external_gateway" {
  type    = string
  default = "533e181f-36be-4f82-a6d2-90fcc2a12185"
}

variable "dns_ip" {
  type    = list(string)
  default = ["8.8.8.8", "8.8.8.4"]
}

#### VM parameters
variable "flavor_http" {
  type    = string
  default = "m1.small"
}

variable "network_http" {
  type = map(string)
  default = {
    subnet_name = "subnet-http"
    cidr        = "192.168.1.0/24"
  }
}

