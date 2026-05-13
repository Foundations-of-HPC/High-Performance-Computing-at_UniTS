# Lecture - OpenStack Advanced: Infrastructure as Code, Images, and HPC Integration

**Course:** High Performance and Cloud Computing 2025/2026
**Programme:** Applied Data Science & Artificial Intelligence - HPC Track
**Institution:** University of Trieste
**Author:** Course instructor
**Duration:** 1 h 45 min (Part 1 ≈ 50 min, Part 2 ≈ 55 min)
---

## Learning Objectives

By the end of this lecture, students will be able to:

1. **Construct** Heat Orchestration Templates (HOT) to declaratively describe and deploy multi-resource OpenStack environments (Bloom: Create)
2. **Configure** the OpenStack Terraform provider and write HCL configurations equivalent to Heat templates (Bloom: Apply)
3. **Compare** Heat and Terraform on multiple dimensions and select the appropriate tool for given scenarios (Bloom: Evaluate)
4. **Build** custom VM images and manage the image lifecycle in Glance (Bloom: Apply)
5. **Design** multi-instance cluster deployments using Terraform count and for_each patterns (Bloom: Create)
6. **Integrate** OpenStack private cloud with HPC batch systems for hybrid burst workflows (Bloom: Synthesize)
7. **Diagnose** common OpenStack deployment failures using quota inspection, console logs, and API debugging techniques (Bloom: Analyze)

---

## Table of Contents

1. [From Manual to Declarative: The Infrastructure-as-Code Imperative](#1-from-manual-to-declarative-the-infrastructure-as-code-imperative)
2. [Heat: OpenStack-Native Orchestration](#2-heat-openstack-native-orchestration)
3. [HOT Template Anatomy](#3-hot-template-anatomy)
4. [Terraform with the OpenStack Provider](#4-terraform-with-the-openstack-provider)
5. [Heat vs. Terraform: Trade-off Analysis](#5-heat-vs-terraform-trade-off-analysis)
6. [Image Management with Glance](#6-image-management-with-glance)
7. [Multi-Instance Cluster Deployments](#7-multi-instance-cluster-deployments)
8. [Object Storage with Swift](#8-object-storage-with-swift)
9. [Integration Patterns with HPC Batch Systems](#9-integration-patterns-with-hpc-batch-systems)
10. [Troubleshooting and Debugging](#10-troubleshooting-and-debugging)
11. [Best Practices for Research Deployments](#11-best-practices-for-research-deployments)
12. [Key Takeaways](#12-key-takeaways)
13. [References](#13-references)
14. [Glossary](#14-glossary)

---

## 1. From Manual to Declarative: The Infrastructure-as-Code Imperative

### 1.1 The Limits of Manual Provisioning

In the Fundamentals lecture, you learned to provision OpenStack resources imperatively: `openstack server create`, `openstack network create`, one command at a time. This approach is adequate for exploratory work, but it is fundamentally unscalable and unscientific for the following reasons:

**Reproducibility**: A cluster provisioned by hand cannot be reliably reproduced. Shell history is not documentation. When a colleague attempts to replicate your environment six months later, they will encounter version mismatches, missing configuration steps, and undocumented security group rules.

**Version control**: You cannot `git diff` a manually configured environment. Infrastructure drift - the gradual divergence between the intended and actual state of a system - is invisible until it causes a failure.

**Automation**: Manual provisioning does not compose with automated workflows. CI/CD pipelines, job schedulers, and benchmarking frameworks need to provision and tear down environments programmatically.

**Auditability**: Research reproducibility requirements increasingly extend to computational infrastructure. Granting agencies and journals may require that the infrastructure configuration used to produce results be archived alongside code and data.

The solution is **Infrastructure as Code (IaC)**: expressing infrastructure configuration in declarative text files that can be version-controlled, reviewed, tested, and applied idempotently. We introduced this concept in the context of Terraform and AWS in Lecture 08. In this lecture, we apply it to OpenStack using two tools: **Heat** (OpenStack-native) and **Terraform** (multi-cloud, with the OpenStack provider).

### 1.2 Declarative vs. Imperative IaC

Recall from Lecture 05 (Kubernetes) the distinction between declarative and imperative control. The same distinction applies here:

| Approach | Model | Example | Characteristic |
|----------|-------|---------|---------------|
| **Imperative** | "Do this, then that" | Shell scripts, Ansible tasks | Order-dependent; fragile to partial failure |
| **Declarative** | "This is the desired state" | Heat HOT, Terraform HCL | Idempotent; tool figures out the sequence |

Both Heat and Terraform are *declarative*: you describe the end state, and the tool determines the sequence of API calls required to achieve it. If part of the state already exists, it is left unchanged; only missing or different resources are modified.

---

## 2. Heat: OpenStack-Native Orchestration

### 2.1 Heat Overview

**Heat** is the OpenStack orchestration service. It was modeled closely on AWS CloudFormation (the same analogy applies here as to the other service pairs) and provides a declarative template language for describing complete cloud environments.

> *Heat is a service to orchestrate composite cloud applications using a declarative template format through an OpenStack-native REST API.* - OpenStack Documentation

A **stack** in Heat is the instantiation of a template - the collection of resources that Heat creates, manages, and (when deleted) destroys atomically. Stacks are first-class objects: you can list them, inspect their resources, and roll back failed updates.

### 2.2 Heat Architecture

```
┌──────────────────────────────────────────────────────────────────────┐
│                        HEAT ARCHITECTURE                             │
│                                                                      │
│   heat-api          heat-api-cfn      heat-api-cloudwatch            │
│   (OpenStack API)   (AWS CF compat.)  (Monitoring compat.)           │
│        │                  │                    │                     │
│   ┌────▼──────────────────▼────────────────────▼──────┐              │
│   │                  heat-engine                      │              │
│   │   - Template parsing and validation               │              │
│   │   - Dependency graph construction (DAG)           │              │
│   │   - Resource lifecycle management                 │              │
│   │   - Event emission                                │              │
│   └────────────────────────┬──────────────────────────┘              │
│                            │                                         │
│              ┌─────────────┼──────────────────┐                      │
│              │             │                  │                      │
│         ┌────▼──┐    ┌─────▼────┐    ┌────────▼────┐                 │
│         │ Nova  │    │ Neutron  │    │  Cinder     │  ... (others)   │
│         └───────┘    └──────────┘    └─────────────┘                 │
└──────────────────────────────────────────────────────────────────────┘
```

Heat builds a **directed acyclic graph (DAG)** of resource dependencies from the template. It provisions resources in topological order: networks before subnets, subnets before routers, routers before instances. This dependency resolution is what distinguishes declarative orchestration from sequential shell scripts.

---

## 3. HOT Template Anatomy

### 3.1 Template Structure

Heat Orchestration Templates (HOT) are YAML files with a well-defined structure:

```yaml
heat_template_version: 2021-04-16   # Identifies supported features

description: |
  Multi-component description of what this template does

parameters:                          # User-supplied variables at deploy time
  param_name:
    type: string | number | json | comma_delimited_list | boolean
    description: Human-readable description
    default: optional_default_value
    constraints:
      - allowed_values: [a, b, c]

conditions:                          # Boolean expressions for conditional resources
  create_prod_resources: { equals: [{get_param: environment}, production] }

resources:                           # The core section: what to create
  resource_logical_name:
    type: OS::ServiceName::ResourceType
    properties:
      property_key: value_or_intrinsic_function
    depends_on: [other_resource]     # Explicit dependencies (usually implicit)

outputs:                             # Values to expose after stack creation
  output_name:
    description: What this value represents
    value: { intrinsic_function }
```

### 3.2 Intrinsic Functions

HOT provides a set of intrinsic functions that enable dynamic value resolution within templates:

| Function | Syntax | Purpose |
|----------|--------|---------|
| `get_param` | `{ get_param: parameter_name }` | Reference a template parameter |
| `get_resource` | `{ get_resource: resource_name }` | Get the ID of another resource in the stack |
| `get_attr` | `{ get_attr: [resource, attr] }` | Get an attribute from a resource |
| `str_replace` | `{ str_replace: {template: "...", params: {}} }` | String interpolation |
| `list_join` | `{ list_join: [",", [a, b, c]] }` | Join a list into a string |
| `if` | `{ if: [condition_name, true_val, false_val] }` | Conditional value |

### 3.3 Complete HOT Example: Research Compute Environment

The following template creates a complete research computing environment: a private network, a router, a head node with a floating IP, and a configurable number of worker nodes. This is a realistic pattern for a small MPI cluster on ADA Cloud.

```yaml
heat_template_version: 2021-04-16

description: |
  Research compute cluster: head node + N worker nodes on a private network.
  Suitable for MPI workloads on CINECA ADA Cloud.

parameters:
  key_name:
    type: string
    description: Name of the SSH keypair to inject into instances
  image_name:
    type: string
    description: Glance image name for all nodes
    default: "Ubuntu 22.04"
  head_flavor:
    type: string
    description: Flavor for the head node
    default: m1.medium
  worker_flavor:
    type: string
    description: Flavor for worker nodes
    default: m1.large
  worker_count:
    type: number
    description: Number of worker nodes
    default: 4
    constraints:
      - range: { min: 1, max: 32 }
  private_cidr:
    type: string
    description: CIDR block for the private network
    default: "10.10.0.0/24"
  external_network:
    type: string
    description: Name of the external provider network
    default: "external-network"

resources:

  # ── Network Infrastructure ──────────────────────────────────────────

  private_net:
    type: OS::Neutron::Net
    properties:
      name: cluster-private-net

  private_subnet:
    type: OS::Neutron::Subnet
    properties:
      network: { get_resource: private_net }
      cidr: { get_param: private_cidr }
      dns_nameservers: ["8.8.8.8", "8.8.4.4"]

  router:
    type: OS::Neutron::Router
    properties:
      external_gateway_info:
        network: { get_param: external_network }

  router_iface:
    type: OS::Neutron::RouterInterface
    properties:
      router: { get_resource: router }
      subnet: { get_resource: private_subnet }

  # ── Security Groups ─────────────────────────────────────────────────

  head_sg:
    type: OS::Neutron::SecurityGroup
    properties:
      name: cluster-head-sg
      rules:
        - direction: ingress
          protocol: tcp
          port_range_min: 22
          port_range_max: 22
          remote_ip_prefix: "0.0.0.0/0"
        - direction: ingress
          protocol: icmp
          remote_ip_prefix: "0.0.0.0/0"

  worker_sg:
    type: OS::Neutron::SecurityGroup
    properties:
      name: cluster-worker-sg
      rules:
        - direction: ingress
          protocol: any
          remote_mode: remote_group_id
          remote_group: { get_resource: head_sg }
        - direction: ingress
          protocol: any
          remote_mode: remote_group_id

  # ── Head Node ────────────────────────────────────────────────────────

  head_node:
    type: OS::Nova::Server
    properties:
      name: cluster-head
      image: { get_param: image_name }
      flavor: { get_param: head_flavor }
      key_name: { get_param: key_name }
      networks:
        - network: { get_resource: private_net }
      security_groups:
        - { get_resource: head_sg }
        - { get_resource: worker_sg }
      user_data_format: RAW
      user_data: |
        #!/bin/bash
        apt-get update -qq
        apt-get install -y openmpi-bin openmpi-common libopenmpi-dev nfs-kernel-server
        mkdir -p /shared
        echo "/shared *(rw,sync,no_subtree_check)" >> /etc/exports
        exportfs -a

  head_fip:
    type: OS::Neutron::FloatingIP
    properties:
      floating_network: { get_param: external_network }

  head_fip_assoc:
    type: OS::Neutron::FloatingIPAssociation
    properties:
      floatingip_id: { get_resource: head_fip }
      port_id: { get_attr: [head_node, addresses, cluster-private-net, 0, port] }

  # ── Worker Nodes ─────────────────────────────────────────────────────

  worker_nodes:
    type: OS::Heat::ResourceGroup
    properties:
      count: { get_param: worker_count }
      resource_def:
        type: OS::Nova::Server
        properties:
          name:
            str_replace:
              template: "cluster-worker-%index%"
              params: {}
          image: { get_param: image_name }
          flavor: { get_param: worker_flavor }
          key_name: { get_param: key_name }
          networks:
            - network: { get_resource: private_net }
          security_groups:
            - { get_resource: worker_sg }
          user_data_format: RAW
          user_data:
            str_replace:
              template: |
                #!/bin/bash
                apt-get update -qq
                apt-get install -y openmpi-bin openmpi-common libopenmpi-dev nfs-common
                mkdir -p /shared
                mount $HEAD_IP:/shared /shared
              params:
                $HEAD_IP: { get_attr: [head_node, first_address] }

outputs:
  head_node_ip:
    description: Public IP address of the cluster head node
    value: { get_attr: [head_fip, floating_ip_address] }

  cluster_private_ips:
    description: Private IP addresses of all worker nodes
    value: { get_attr: [worker_nodes, first_address] }

  ssh_command:
    description: SSH command to access the head node
    value:
      str_replace:
        template: "ssh ubuntu@$IP"
        params:
          $IP: { get_attr: [head_fip, floating_ip_address] }
```

### 3.4 Heat CLI Operations

```bash
# Validate a template without creating resources
openstack stack create --dry-run -t cluster.yaml test-validate

# Create a stack, providing parameters
openstack stack create \
  -t cluster.yaml \
  --parameter key_name=my-keypair \
  --parameter worker_count=4 \
  --parameter image_name="Ubuntu 22.04" \
  research-cluster

# Monitor stack creation (watch resource states)
openstack stack show research-cluster
openstack stack resource list research-cluster
openstack stack event list research-cluster

# Update a stack (e.g., scale to 8 workers)
openstack stack update \
  -t cluster.yaml \
  --parameter worker_count=8 \
  research-cluster

# Delete a stack (removes ALL resources created by it)
openstack stack delete research-cluster
```

**Important**: `openstack stack delete` removes *all* resources in the stack, including networks, floating IPs, and volumes. This is intentional and ensures no orphaned resources accumulate - but it means you must ensure any important data is copied out before deletion.

---
## 4. Terraform
### How Terraform Works

Terraform operates on a simple but powerful workflow that mediates between your desired infrastructure (expressed in HCL configuration files) and the actual state of resources in your cloud provider. The following diagram illustrates this workflow:

```
┌─────────────────────────────────────────────────────────────────────┐
│                      TERRAFORM WORKFLOW                             │
│                                                                     │
│   ┌──────────────┐                                                  │
│   │   .tf files  │  ← You write HCL configuration                   │
│   │  (HCL code)  │    describing desired infrastructure             │
│   └──────┬───────┘                                                  │
│          │                                                          │
│          ▼                                                          │
│   ┌──────────────┐     ┌────────────────┐                           │
│   │   terraform  │────►│   Provider     │  ← Plugins for AWS,       │
│   │     init     │     │   Downloads    │    Azure, GCP, K8s...     │
│   └──────┬───────┘     └────────────────┘                           │
│          │                                                          │
│          ▼                                                          │
│   ┌──────────────┐     ┌────────────────┐                           │
│   │   terraform  │────►│  State File    │  ← What currently exists  │
│   │     plan     │     │  (tfstate)     │    in the real world      │
│   └──────┬───────┘     └────────────────┘                           │
│          │                                                          │
│          │  Shows: "+3 to add, ~1 to change, -0 to destroy"         │
│          ▼                                                          │
│   ┌──────────────┐     ┌────────────────┐                           │
│   │   terraform  │────►│  Cloud APIs    │  ← Creates/updates/       │
│   │    apply     │     │  (AWS, etc.)   │    deletes real resources │
│   └──────┬───────┘     └────────────────┘                           │
│          │                                                          │
│          ▼                                                          │
│   ┌──────────────┐                                                  │
│   │  terraform   │  ← Records what was created:                     │
│   │  .tfstate    │    resource IDs, attributes, dependencies        │
│   │  (updated)   │    CRITICAL - do not lose or corrupt this file   │
│   └──────────────┘                                                  │
└─────────────────────────────────────────────────────────────────────┘
```

This workflow embodies the same **control loop** pattern you studied in Kubernetes (L05): *desired state → compare → current state → reconcile*. Terraform reads your configuration (desired state), queries the cloud provider via the state file and API calls (current state), computes the difference (plan), and applies the minimal set of changes to converge the two (apply).

### Core Concepts

| Concept | Description | Analogy |
|---------|-------------|---------|
| **Configuration** | `.tf` files defining desired infrastructure | Source code |
| **Provider** | Plugin for a specific platform (AWS, Azure, K8s) | Device driver |
| **Resource** | A single infrastructure object (EC2 instance, VPC, S3 bucket) | Object instance |
| **Data Source** | Read-only query of existing infrastructure | Database SELECT |
| **State** | Mapping between your config and real-world resources | Database / inventory |
| **Module** | Reusable, encapsulated package of configuration | Function / library |
| **Backend** | Where state is stored (local file, S3, Terraform Cloud) | Database backend |

### CLI Commands

The Terraform CLI provides a focused set of commands that map to the workflow stages:

| Command | Purpose | When to Use |
|---------|---------|-------------|
| `terraform init` | Initialize working directory, download providers and modules | First time, or after adding providers/modules |
| `terraform plan` | Preview changes without applying (dry run) | Before every apply; in CI/CD pipelines |
| `terraform apply` | Create, update, or delete resources to match configuration | After reviewing the plan |
| `terraform destroy` | Delete all resources managed by the configuration | Cleanup; end of exercise |
| `terraform fmt` | Reformat code to canonical style | Before committing; in CI checks |
| `terraform validate` | Check configuration for syntax and internal consistency | After writing; in CI |
| `terraform output` | Display output values from state | To retrieve IPs, IDs, connection strings |
| `terraform state` | Advanced state inspection and manipulation | Debugging; import; refactoring |
| `terraform import` | Bring existing resources under Terraform management | Adopting manually-created resources |
| `terraform taint` | Mark a resource for recreation on next apply | Force replacement (deprecated; use `-replace`) |

### Installation

Terraform is distributed as a single static binary. On Ubuntu/Debian systems, the recommended installation uses the HashiCorp APT repository:

```bash
# Add the HashiCorp GPG key and repository
wget -O - https://apt.releases.hashicorp.com/gpg | \
  sudo gpg --dearmor -o /usr/share/keyrings/hashicorp-archive-keyring.gpg

echo "deb [arch=$(dpkg --print-architecture) \
  signed-by=/usr/share/keyrings/hashicorp-archive-keyring.gpg] \
  https://apt.releases.hashicorp.com $(lsb_release -cs) main" | \
  sudo tee /etc/apt/sources.list.d/hashicorp.list

sudo apt update && sudo apt install terraform

# Verify installation
terraform version
# Terraform v1.14.5

# Enable shell tab completion
terraform -install-autocomplete
```

### What is HCL?

**HashiCorp Configuration Language (HCL)** is a declarative language specifically designed for infrastructure definition. It occupies a deliberate middle ground: more structured than YAML (which lacks expressions, conditionals, and functions), but more approachable than a general-purpose language like Python (which requires understanding control flow, error handling, and state management). HCL is human-readable, machine-parseable, and supports the abstractions that infrastructure configuration demands.

### Block Syntax

The fundamental unit of HCL is the **block**. Every block follows the pattern `TYPE "LABEL1" "LABEL2" { ... }`:

```hcl
# A resource block: type is "resource", labels are the resource type and local name
resource "aws_instance" "web_server" {
  ami           = "ami-0123456789abcdef0"
  instance_type = "t3.micro"

  tags = {
    Name        = "WebServer"
    Environment = "Production"
  }
}
```

The **resource type** (`aws_instance`) identifies the infrastructure object in the provider's API. The **local name** (`web_server`) is an identifier used within your Terraform configuration to reference this specific resource - it has no meaning outside your code.

### Data Types and Expressions

HCL supports a rich type system:

```hcl
# Primitive types
name    = "my-instance"      # string
count   = 3                  # number
enabled = true               # boolean

# Collection types
availability_zones = ["eu-west-1a", "eu-west-1b", "eu-west-1c"]   # list(string)

tags = {                     # map(string)
  Name = "WebServer"
  Env  = "Production"
}

# Structural types (object, tuple) - used in variable definitions
variable "server_config" {
  type = object({
    instance_type = string
    volume_size   = number
    encrypted     = bool
  })
}

# Heredoc for multi-line strings
user_data = <<-EOF
  #!/bin/bash
  yum update -y
  yum install -y docker
  systemctl start docker
  systemctl enable docker
EOF
```

### References and Expressions

HCL's expression system enables resources to reference each other, creating the dependency graph that Terraform uses to determine execution order:

```hcl
# Reference another resource's attribute
subnet_id = aws_subnet.public.id

# Reference a variable
instance_type = var.instance_type

# Reference a data source
ami = data.aws_ami.amazon_linux.id

# Reference a module output
vpc_id = module.networking.vpc_id

# String interpolation
name = "web-${var.environment}-${count.index + 1}"

# Conditional expression (ternary)
instance_type = var.environment == "prod" ? "t3.large" : "t3.micro"

# For expressions (list comprehension)
upper_names = [for name in var.names : upper(name)]

# For expressions (map construction)
instance_map = { for inst in aws_instance.web : inst.tags.Name => inst.id }

# Splat expression (shorthand for simple attribute extraction)
subnet_ids = aws_subnet.public[*].id
```

### Built-in Functions

Terraform provides a comprehensive function library. You cannot define custom functions in HCL, but the built-in set covers the vast majority of infrastructure configuration needs:

| Category | Functions | Example |
|----------|-----------|---------|
| **String** | `lower()`, `upper()`, `trim()`, `replace()`, `split()`, `join()` | `lower("HELLO")` → `"hello"` |
| **Collection** | `length()`, `concat()`, `flatten()`, `merge()`, `lookup()`, `keys()`, `values()` | `length(["a","b"])` → `2` |
| **Filesystem** | `file()`, `templatefile()`, `fileexists()`, `filebase64()` | `file("scripts/init.sh")` |
| **Encoding** | `base64encode()`, `base64decode()`, `jsonencode()`, `jsondecode()`, `yamlencode()` | `jsonencode({a = 1})` → `"{\"a\":1}"` |
| **Network** | `cidrsubnet()`, `cidrhost()`, `cidrnetmask()` | `cidrsubnet("10.0.0.0/16", 8, 1)` → `"10.0.1.0/24"` |
| **Date/Time** | `timestamp()`, `formatdate()`, `timeadd()` | `formatdate("YYYY-MM-DD", timestamp())` |
| **Type** | `tostring()`, `tolist()`, `tomap()`, `try()`, `can()` | `try(var.x, "default")` |
| **Hash/Crypto** | `md5()`, `sha256()`, `bcrypt()`, `uuid()` | `sha256("data")` |

The `cidrsubnet()` function deserves special attention for cloud networking. It computes subnet CIDR blocks arithmetically from a parent CIDR, which is essential for creating multiple subnets programmatically:

```hcl
# Given VPC CIDR 10.0.0.0/16, create /24 subnets
# cidrsubnet(prefix, newbits, netnum)
# newbits: how many bits to add to the prefix (16 + 8 = /24)
# netnum: which subnet number (0, 1, 2, ...)

resource "aws_subnet" "public" {
  count      = 3
  cidr_block = cidrsubnet("10.0.0.0/16", 8, count.index)
  # Result: 10.0.0.0/24, 10.0.1.0/24, 10.0.2.0/24
}
```

### Dynamic Blocks

When a resource requires a repeated nested block (e.g., multiple ingress rules in a security group), **dynamic blocks** avoid duplication:

```hcl
variable "ingress_rules" {
  type = list(object({
    port        = number
    description = string
    cidr_blocks = list(string)
  }))
  default = [
    { port = 22,  description = "SSH",  cidr_blocks = ["10.0.0.0/8"] },
    { port = 80,  description = "HTTP", cidr_blocks = ["0.0.0.0/0"] },
    { port = 443, description = "HTTPS", cidr_blocks = ["0.0.0.0/0"] },
  ]
}

resource "aws_security_group" "web" {
  name        = "web-sg"
  description = "Web server security group"
  vpc_id      = aws_vpc.main.id

  dynamic "ingress" {
    for_each = var.ingress_rules
    content {
      from_port   = ingress.value.port
      to_port     = ingress.value.port
      protocol    = "tcp"
      cidr_blocks = ingress.value.cidr_blocks
      description = ingress.value.description
    }
  }

  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
}
```

> **When to use dynamic blocks**: Only when the number of nested blocks is genuinely variable (driven by input data). Do not use dynamic blocks for a fixed set of rules - explicit blocks are clearer. Over-use of dynamic blocks makes configuration harder to read and debug.

### What is Terraform State?

The **state file** (`terraform.tfstate`) is the critical data structure that maps your HCL configuration to real-world infrastructure. It is a JSON file containing resource IDs, attribute values, dependency information, and metadata. Without the state file, Terraform has no way to know which cloud resources correspond to which configuration blocks.

```
┌───────────────────────────────────────────────────────────┐
│                  STATE FILE ROLE                          │
│                                                           │
│   .tf Configuration          State File         Cloud     │
│   ┌─────────────┐      ┌─────────────────┐    ┌───────┐   │
│   │ resource    │      │ "aws_instance"  │    │ EC2   │   │
│   │ "aws_inst"  │◄────►│ id: i-0abc123   │◄──►│ i-... │   │
│   │ "web"       │      │ type: t3.micro  │    │       │   │
│   └─────────────┘      │ ip: 203.0.113.5 │    └───────┘   │
│                        └─────────────────┘                │
│                                                           │
│   Terraform uses state to:                                │
│   • Know which real resource matches each config block    │
│   • Detect drift (config vs. reality)                     │
│   • Compute the plan (what to create/change/delete)       │
│   • Track dependencies for ordered operations             │
└───────────────────────────────────────────────────────────┘
```

**Critical warning**: If you lose the state file, Terraform cannot manage existing resources. They continue to run and incur costs, but Terraform treats your configuration as entirely new and will attempt to create duplicate resources. Recovery requires `terraform import` for each resource, which is tedious and error-prone.

### 4.1 Why Terraform on OpenStack?

The same HCL syntax, state management model, and operational workflow apply directly to OpenStack via the **OpenStack Terraform Provider** (`terraform-provider-openstack/openstack`). Using Terraform rather than Heat has a key advantage for your career: the skills transfer to any cloud provider.

### 4.2 Provider Configuration

```hcl
# versions.tf
terraform {
  required_version = ">= 1.5.0"
  required_providers {
    openstack = {
      source  = "terraform-provider-openstack/openstack"
      version = "~> 1.54"
    }
  }
}

# provider.tf
# Credentials are read from environment variables set by sourcing openrc.sh
# These map to: OS_AUTH_URL, OS_APPLICATION_CREDENTIAL_ID,
#               OS_APPLICATION_CREDENTIAL_SECRET, OS_REGION_NAME
provider "openstack" {
  # If using a custom CA certificate (CINECA ADA):
  # cacert_file = "/path/to/cineca-chain.pem"
}
```

```bash
# Initialization
terraform init    # Downloads the OpenStack provider plugin
terraform plan    # Shows what will be created/changed/destroyed
terraform apply   # Applies the plan (creates resources)
terraform destroy # Destroys all managed resources
```

### 4.3 Data Sources and Resources

Terraform distinguishes **data sources** (read existing infrastructure) from **resources** (create/manage infrastructure). This is analogous to the difference between Terraform reading an AMI ID vs. creating an EC2 instance.

```hcl
# Data sources - query existing OpenStack resources
data "openstack_images_image_v2" "ubuntu" {
  name        = "Ubuntu_24.04"
  most_recent = true
}

data "openstack_networking_network_v2" "external" {
  name = "external-network"  # Must match the provider network name in ADA
}

data "openstack_compute_flavor_v2" "medium" {
  name = "m1.medium"
}
```

### 4.4 Complete Terraform Example: Research Cluster

```hcl
# variables.tf
variable "key_name" {
  description = "SSH keypair registered in OpenStack"
  type        = string
}

variable "worker_count" {
  description = "Number of worker nodes"
  type        = number
  default     = 4
}

variable "worker_flavor" {
  description = "Flavor for worker nodes"
  type        = string
  default     = "m1.large"
}

# main.tf - Network infrastructure
resource "openstack_networking_network_v2" "cluster_net" {
  name           = "cluster-network"
  admin_state_up = true
}

resource "openstack_networking_subnet_v2" "cluster_subnet" {
  name            = "cluster-subnet"
  network_id      = openstack_networking_network_v2.cluster_net.id
  cidr            = "10.10.0.0/24"
  ip_version      = 4
  dns_nameservers = ["8.8.8.8", "8.8.4.4"]
}

resource "openstack_networking_router_v2" "router" {
  name                = "cluster-router"
  external_network_id = data.openstack_networking_network_v2.external.id
}

resource "openstack_networking_router_interface_v2" "router_iface" {
  router_id = openstack_networking_router_v2.router.id
  subnet_id = openstack_networking_subnet_v2.cluster_subnet.id
}

# Security groups
resource "openstack_networking_secgroup_v2" "head_sg" {
  name        = "cluster-head-sg"
  description = "Security group for cluster head node"
}

resource "openstack_networking_secgroup_rule_v2" "ssh_ingress" {
  direction         = "ingress"
  ethertype         = "IPv4"
  protocol          = "tcp"
  port_range_min    = 22
  port_range_max    = 22
  remote_ip_prefix  = "0.0.0.0/0"
  security_group_id = openstack_networking_secgroup_v2.head_sg.id
}

resource "openstack_networking_secgroup_rule_v2" "icmp_ingress" {
  direction         = "ingress"
  ethertype         = "IPv4"
  protocol          = "icmp"
  remote_ip_prefix  = "0.0.0.0/0"
  security_group_id = openstack_networking_secgroup_v2.head_sg.id
}

resource "openstack_networking_secgroup_v2" "worker_sg" {
  name        = "cluster-worker-sg"
  description = "Security group for worker nodes"
}

resource "openstack_networking_secgroup_rule_v2" "intra_cluster" {
  direction              = "ingress"
  ethertype              = "IPv4"
  remote_group_id        = openstack_networking_secgroup_v2.worker_sg.id
  security_group_id      = openstack_networking_secgroup_v2.worker_sg.id
}

# Keypair
resource "openstack_compute_keypair_v2" "keypair" {
  name       = var.key_name
  public_key = file("~/.ssh/id_rsa.pub")
}

# Head node
resource "openstack_compute_instance_v2" "head" {
  name            = "cluster-head"
  image_id        = data.openstack_images_image_v2.ubuntu.id
  flavor_id       = data.openstack_compute_flavor_v2.medium.id
  key_pair        = openstack_compute_keypair_v2.keypair.name
  security_groups = [
    openstack_networking_secgroup_v2.head_sg.name,
    openstack_networking_secgroup_v2.worker_sg.name
  ]

  network {
    uuid = openstack_networking_network_v2.cluster_net.id
  }

  user_data = <<-EOF
    #!/bin/bash
    apt-get update -qq
    apt-get install -y openmpi-bin openmpi-common libopenmpi-dev nfs-kernel-server
    mkdir -p /shared
    echo "/shared *(rw,sync,no_subtree_check)" >> /etc/exports
    exportfs -a
  EOF
}

resource "openstack_networking_floatingip_v2" "head_fip" {
  pool = "external-network"
}

resource "openstack_compute_floatingip_associate_v2" "head_fip_assoc" {
  floating_ip = openstack_networking_floatingip_v2.head_fip.address
  instance_id = openstack_compute_instance_v2.head.id
}

# Worker nodes using count
resource "openstack_compute_instance_v2" "workers" {
  count           = var.worker_count
  name            = "cluster-worker-${count.index + 1}"
  image_id        = data.openstack_images_image_v2.ubuntu.id
  flavor_name     = var.worker_flavor
  key_pair        = openstack_compute_keypair_v2.keypair.name
  security_groups = [openstack_networking_secgroup_v2.worker_sg.name]

  network {
    uuid = openstack_networking_network_v2.cluster_net.id
  }

  user_data = <<-EOF
    #!/bin/bash
    apt-get update -qq
    apt-get install -y openmpi-bin openmpi-common libopenmpi-dev nfs-common
    mkdir -p /shared
    echo "${openstack_compute_instance_v2.head.access_ip_v4}:/shared /shared nfs defaults 0 0" >> /etc/fstab
    mount -a
  EOF

  depends_on = [openstack_compute_instance_v2.head]
}

# Shared research data volume attached to head node
resource "openstack_blockstorage_volume_v3" "shared_data" {
  name        = "cluster-shared-data"
  size        = 500
  description = "Shared simulation input/output storage"
}

resource "openstack_compute_volume_attach_v2" "data_attach" {
  instance_id = openstack_compute_instance_v2.head.id
  volume_id   = openstack_blockstorage_volume_v3.shared_data.id
}

# outputs.tf
output "head_public_ip" {
  description = "Public IP of cluster head node"
  value       = openstack_networking_floatingip_v2.head_fip.address
}

output "worker_private_ips" {
  description = "Private IPs of worker nodes"
  value       = openstack_compute_instance_v2.workers[*].access_ip_v4
}

output "ssh_command" {
  description = "SSH command to access head node"
  value       = "ssh ubuntu@${openstack_networking_floatingip_v2.head_fip.address}"
}
```

---

## 5. Heat vs. Terraform: Trade-off Analysis

This is a question with genuine architectural implications, not just personal preference. The choice depends on your operational context.

| Dimension | Heat | Terraform |
|-----------|------|-----------|
| **Native OpenStack integration** | Yes - first-class citizen | Via provider plugin |
| **Template language** | YAML (HOT) | HCL |
| **State management** | Stored in OpenStack DB (Heat engine) | Local file or remote backend |
| **Drift detection** | Stack events, resource list | `terraform plan` shows drift |
| **Multi-cloud support** | OpenStack only | AWS, Azure, GCP, OpenStack, etc. |
| **Skill transferability** | Limited to OpenStack | High - same tool across clouds |
| **AWS CloudFormation compatibility** | Yes (heat-api-cfn endpoint) | N/A |
| **Template composability** | Nested stacks, `OS::Heat::ResourceGroup` | Modules, `count`, `for_each` |
| **Community and ecosystem** | OpenStack community | Very large, multi-cloud |
| **Rollback on failure** | Automatic (configurable) | Manual (requires state manipulation) |
| **Dry-run capability** | `--dry-run` flag | `terraform plan` |

**Recommendation**: For CINECA ADA and other OpenStack-only environments, both tools are viable. **Heat** is appropriate when tight integration with OpenStack's stack lifecycle management is required, or when operators already manage Heat. **Terraform** is preferable when your workflows span multiple cloud environments, when you have existing Terraform expertise, or when you want skills that transfer directly to AWS/Azure/GCP work.

In practice, research teams often use Terraform for reproducible environment provisioning and reserve Heat for platform-level operations performed by cloud administrators.

---

## 6. Image Management with Glance

### 6.1 The Case for Custom Images

Cloud images distributed by Linux vendors (Ubuntu Cloud Images, CentOS Cloud, etc.) are minimal and generic - they contain no HPC software, no site-specific configuration, and no research tools. Downloading and installing all required software on every VM at boot time is slow (minutes of `apt-get install` at VM startup) and creates unnecessary network load.

**Custom images** solve this by baking software installations into the image itself. A well-designed custom image approach follows the principle: *install once, boot many times*.

### 6.2 Building and Uploading Custom Images

```bash
# Option 1: Start from an existing cloud image and snapshot
# 1. Launch an instance from a base image
openstack server create \
  --image "Ubuntu 22.04" \
  --flavor m1.medium \
  --network my-network \
  --key-name my-keypair \
  image-builder

# 2. SSH in and install software
ssh ubuntu@<floating-ip>
sudo apt-get update && sudo apt-get upgrade -y
sudo apt-get install -y \
  openmpi-bin libopenmpi-dev \
  python3-numpy python3-scipy python3-matplotlib \
  htop iotop nfs-common parallel



# Clean up before snapshotting
sudo apt-get clean
sudo cloud-init clean --logs
sudo rm -rf /tmp/* /var/tmp/*

# 3. Create a snapshot from the running/stopped instance
openstack server stop image-builder
openstack server image create \
  --name "Ubuntu-22.04-HPC-v1.0" \
  image-builder

# Monitor image creation (may take several minutes)
openstack image list --name "Ubuntu-22.04-HPC-v1.0"

# 4. Verify image is active before using
openstack image show "Ubuntu-22.04-HPC-v1.0"

# Option 2: Upload a pre-built cloud image (e.g., from cloud-images.ubuntu.com)
wget https://cloud-images.ubuntu.com/jammy/current/jammy-server-cloudimg-amd64.img

openstack image create \
  --file jammy-server-cloudimg-amd64.img \
  --disk-format qcow2 \
  --container-format bare \
  --min-ram 512 \
  --min-disk 3 \
  "Ubuntu-22.04-Official"
```

### 6.3 Image Lifecycle Best Practices

| Practice | Rationale |
|----------|-----------|
| Version images with semantic names | `Ubuntu-22.04-HPC-v1.2` rather than overwriting `HPC-image` |
| Document installed software in image metadata | Use `openstack image set --property installed_software=...` |
| Store image build scripts in version control | Reproducibility - anyone can rebuild the image |
| Build images from automation (Packer) | Eliminates manual steps, enables CI/CD of images |
| Test images before marking them "official" | Boot a test VM, run a smoke test, then mark as production |

---

## 7. Multi-Instance Cluster Deployments

### 7.1 ResourceGroup in Heat

Heat's `OS::Heat::ResourceGroup` allows creating multiple identical resources with a single definition. Combined with `%index%` substitution for naming, this is the Heat equivalent of Terraform's `count`:

```yaml
worker_nodes:
  type: OS::Heat::ResourceGroup
  properties:
    count: { get_param: worker_count }
    resource_def:
      type: OS::Nova::Server
      properties:
        name:
          str_replace:
            template: "worker-%index%"
            params: {}
        # ... remaining properties
```

### 7.2 Generating Host Files for MPI

For MPI workloads, worker nodes need to know each other's addresses. A common pattern uses `cloud-init` on the head node to dynamically generate an MPI hostfile:

```hcl
# Generate an MPI hostfile from Terraform worker IPs
resource "local_file" "mpi_hostfile" {
  content = join("\n", [
    for ip in openstack_compute_instance_v2.workers[*].access_ip_v4 :
    "${ip} slots=4"
  ])
  filename = "hostfile"
}

output "mpi_run_command" {
  value = "mpirun --hostfile hostfile -np ${var.worker_count * 4} ./my_simulation"
}
```

### 7.3 NFS Shared Filesystem Pattern

A common pattern for small HPC clusters on OpenStack: the head node exports `/shared` via NFS; all workers mount it. This provides a shared workspace for input files, checkpoints, and output - analogous to a simplified Lustre setup.

```
┌──────────────────────────────────────────────────────────────────────┐
│                 SHARED FILESYSTEM PATTERN                            │
│                                                                      │
│    ┌──────────────────────┐                                          │
│    │    HEAD NODE         │                                          │
│    │  ┌───────────────┐   │      Cinder Volume                       │
│    │  │  NFS Server   │◄──┼──────[/dev/vdb → /shared/data]           │
│    │  │  /shared      │   │      (persistent, 500GB)                 │
│    │  └──────┬────────┘   │                                          │
│    └─────────┼────────────┘                                          │
│              │  NFS exports /shared                                  │
│    ┌─────────┼────────────────────────────────────────┐              │
│    │         │              Private Network           │              │
│    │   ┌─────▼──┐  ┌────────┐  ┌────────┐  ┌──────┐   │              │
│    │   │Worker 1│  │Worker 2│  │Worker 3│  │  ... │   │              │
│    │   │/shared/│  │/shared/│  │/shared/│  │      │   │              │
│    │   │(NFS   )│  │(NFS   )│  │(NFS   )│  │      │   │              │
│    │   └────────┘  └────────┘  └────────┘  └──────┘   │              │
│    └──────────────────────────────────────────────────┘              │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 8. Object Storage with Swift

### 8.1 Swift Overview

**Swift** is OpenStack's object storage service, functionally equivalent to Amazon S3. Objects are organized into **containers** (analogous to S3 buckets) and accessed via a RESTful HTTP API. Swift provides a distributed, eventually consistent storage system appropriate for large unstructured data - genome sequences, simulation snapshots, raw sensor data.

```bash
# Create a container
openstack container create simulation-inputs
openstack container create simulation-outputs

# Upload files to object storage
openstack object create simulation-inputs reference-genome.fa
openstack object create simulation-inputs parameters.json

# List container contents
openstack object list simulation-inputs

# Download a specific object
openstack object save simulation-outputs results-run42.tar.gz

# Upload a directory of files (using Python Swift client)
pip install python-swiftclient
swift upload simulation-inputs ./input-data/

# Generate a temporary URL for sharing (time-limited access)
openstack object store account set --property Temp-URL-Key=<random-secret>
swift tempurl GET 3600 /v1/AUTH_<project-id>/simulation-inputs/results.tar.gz <secret>

# Delete objects and containers
openstack object delete simulation-inputs old-params.json
openstack container delete --recursive old-container
```

### 8.2 Swift for HPC Data Staging

Swift serves a natural role as a **data staging layer** in hybrid HPC-cloud workflows:

| Use Case | Swift Pattern |
|----------|-------------|
| Pre-stage input data | Upload reference databases, parameter files to Swift before launching VMs |
| Checkpoint storage | Periodically write simulation checkpoints to Swift from VMs |
| Results archival | Write final outputs to Swift; fetch from HPC system via `swift download` |
| Data sharing | Generate temporary URLs for collaborators without granting full OpenStack access |

---

## 9. Integration Patterns with HPC Batch Systems

### 9.1 The Hybrid HPC-Cloud Architecture

For  students, the most relevant use case for OpenStack is not replacing HPC systems but *complementing* them. Several integration patterns are relevant:

**Pattern 1: Cloud for Interactive Analysis**

CINECA's HPC systems (Leonardo, Galileo) are optimized for batch computation and have strict policies on interactive use. ADA Cloud provides a complementary environment where researchers can:
- Perform exploratory data analysis interactively
- Test code before submitting HPC batch jobs
- Run Jupyter notebooks for result visualization
- Host web-based scientific portals

**Pattern 2: Pre/Post-Processing on Cloud**

Some simulation workflows involve asymmetric computational phases:

```
┌─────────────────────────────────────────────────────────────────────┐
│              HYBRID WORKFLOW: CLOUD + HPC                           │
│                                                                     │
│   ADA Cloud (OpenStack)              Leonardo (SLURM)               │
│   ┌──────────────────┐               ┌──────────────────┐           │
│   │  Pre-processing  │               │  Main Simulation │           │
│   │  - Data cleaning │    ──────►    │  - MPI parallel  │           │
│   │  - Format conv.  │  data via     │  - GPU compute   │           │
│   │  - Param sweep   │  Swift/SFTP   │  - Large memory  │           │
│   └──────────────────┘               └──────────┬───────┘           │
│                                                 │                   │
│   ADA Cloud (OpenStack)                         │                   │
│   ┌──────────────────┐               ◄───────── ┘                   │
│   │  Post-processing │  data via                                    │
│   │  - Visualization │  Swift/SFTP                                  │
│   │  - Statistical   │                                              │
│   │    analysis      │                                              │
│   └──────────────────┘                                              │
└─────────────────────────────────────────────────────────────────────┘
```

**Pattern 3: Cloud Bursting**

When HPC queues are long, researchers can burst overflow workloads to ADA Cloud. This requires workloads that can run on OpenStack VMs (without InfiniBand or specialized hardware) - embarrassingly parallel parameter sweeps are the canonical example.

### 9.2 Practical Cloud Bursting Example

```bash
# On CINECA HPC system (Leonardo/Galileo):
# Submit primary job to SLURM
sbatch primary-simulation.slurm

# Meanwhile, provision ADA Cloud for embarrassingly parallel sweep
source ~/openstack-credentials/app-cred-openrc.sh
terraform apply -var="worker_count=16" -auto-approve

# Stage parameter files to ADA workers via Swift
openstack object create param-sweep/ params_000.json
openstack object create param-sweep/ params_001.json
# ... (or use a script to upload all parameter files)

# On each ADA worker (via cloud-init or Ansible):
# - Download parameter file from Swift
# - Run simulation
# - Upload results to Swift

# Fetch all results back to HPC filesystem
swift download param-sweep-results --output-dir /scratch/$USER/results/

# Tear down ADA infrastructure
terraform destroy -auto-approve
```

### 9.3 SSH Configuration for ADA Cluster Access

A practical SSH configuration for working with an ADA Cloud cluster:

```
# ~/.ssh/config
Host ada-head
  HostName <floating-ip>
  User ubuntu
  IdentityFile ~/.ssh/id_rsa
  StrictHostKeyChecking no
  UserKnownHostsFile /dev/null

# Jump through head node to reach workers
Host ada-worker-*
  User ubuntu
  IdentityFile ~/.ssh/id_rsa
  ProxyJump ada-head
  StrictHostKeyChecking no
```

---

## 10. Troubleshooting and Debugging

### 10.1 Common Failure Modes

| Symptom | Likely Cause | Diagnostic Command | Solution |
|---------|-------------|-------------------|---------|
| `Authentication error (401)` | Invalid/expired credentials | `openstack token issue` | Re-source openrc file; regenerate App Credential |
| Instance stuck in `BUILD` | Scheduler cannot find a host | `openstack server show <instance>` | Check quotas; try a smaller flavor |
| Instance enters `ERROR` state | Boot failure | `openstack console log show <instance>` | Check cloud-init errors in console log |
| Cannot ping instance | Security group missing ICMP | `openstack security group rule list` | Add ICMP ingress rule |
| Cannot SSH to instance | Missing SSH rule or no floating IP | `openstack floating ip list` | Check SG and floating IP assignment |
| Volume attach fails | Volume/instance in different AZ | `openstack volume show <volume>` | Create volume in same AZ as instance |
| Floating IP allocation fails | External IP pool exhausted | `openstack quota show` | Contact CINECA admin |
| Stack creation fails (Heat) | Resource dependency error | `openstack stack event list <stack>` | Inspect failed resource events |
| `terraform apply` fails | State mismatch or API error | `terraform show` | Run `terraform plan` to assess state |

### 10.2 Quota Management

Quotas are limits on resource consumption per project. Exceeding quotas causes resource creation to fail with `OverQuota` errors.

```bash
# View all quotas for your project
openstack quota show

# Check current usage vs. limits
openstack limits show --absolute

# Key quota dimensions to monitor:
# - instances: number of VMs
# - cores: total vCPU count across all VMs
# - ram: total RAM in MB
# - floating_ips: floating IP address count
# - volumes: number of Cinder volumes
# - gigabytes: total volume storage in GB
# - networks, subnets, routers, ports
```

### 10.3 Debugging Instance Boot Issues

```bash
# Step 1: Check instance state
openstack server show my-instance | grep status

# Step 2: Check console output (cloud-init logs visible here)
openstack console log show my-instance | tail -50

# Step 3: Open a browser console (noVNC) for interactive debugging
openstack console url show --type novnc my-instance

# Step 4: If instance is unreachable, verify network configuration
openstack port list --server my-instance
openstack floating ip list

# Step 5: Verify security group rules
openstack security group rule list <group-id>

# Step 6: For Terraform, inspect state
terraform state list
terraform state show openstack_compute_instance_v2.head
```

---

## 11. Best Practices for Research Deployments

### 11.1 Infrastructure as Code Always

**Never provision research infrastructure manually** if it will be used for publishable results. Every VM, volume, network, and security group should be described in a Heat template or Terraform configuration that is committed to version control alongside your code and data.

### 11.2 Authentication Security

| Practice | Rationale |
|----------|-----------|
| Use Application Credentials, never passwords | Application Credentials are scoped, revocable, and don't expose user credentials |
| Store credentials in environment variables, not files | Prevents accidental credential commits to version control |
| Add `*.env` and `*openrc*.sh` to `.gitignore` | Ensure credentials are never committed |
| Rotate Application Credentials periodically | Limit exposure window if a credential is compromised |
| Use separate credentials per project | Limits blast radius of credential compromise |

### 11.3 Resource Tagging

OpenStack supports arbitrary metadata on most resources. Use this to track ownership and purpose:

```bash
openstack server set --property owner=$USER --property project=genome-analysis my-instance
openstack volume set --property purpose=simulation-data research-vol
```

### 11.4 Cost and Quota Awareness

Unlike AWS, CINECA ADA does not bill per compute hour - but you do have quotas. Treat quotas as shared resources:

- **Always delete resources when not in use**: `terraform destroy` or `openstack stack delete`
- **Use the smallest flavor that meets your requirements** during development
- **Do not leave instances running overnight** during development/testing
- **Monitor your quota usage** with `openstack limits show --absolute`

### 11.5 Data Durability

| Data Category | Storage Strategy |
|---------------|-----------------|
| Input datasets | Cinder volume attached to head; backup to Swift |
| Active simulation state | Cinder volume; periodic checkpoints to Swift |
| Results/outputs | Swift (durable, accessible after stack deletion) |
| Code and configuration | Git repository (never store in VM ephemeral disk alone) |
| VM configuration | Heat template or Terraform state in Git |

---

## 12. Key Takeaways

### Part 1 Summary (Infrastructure as Code)

1. **IaC is mandatory** for reproducible research computing. Manual provisioning is documentation debt; a Heat template or Terraform configuration *is* the documentation.

2. **Heat** provides OpenStack-native declarative orchestration via HOT templates. Stack lifecycle management (create, update, delete as a unit) is its key advantage. `OS::Heat::ResourceGroup` enables parameterized multi-instance deployments.

3. **Terraform** with the OpenStack provider applies the same HCL syntax you learned for AWS, enabling multi-cloud skill transfer. The `count` and `for_each` patterns are the Terraform equivalent of `ResourceGroup`.

4. **Choose based on context**: Heat for OpenStack-only, platform-managed stacks; Terraform for multi-cloud portability and teams with existing Terraform expertise.

### Part 2 Summary (Images, Clusters, Integration)

5. **Custom images** eliminate boot-time software installation delays and ensure environment reproducibility. Version images, document contents, and store build scripts in version control.

6. **Multi-instance clusters**: Both Heat (`ResourceGroup`) and Terraform (`count`) support scalable cluster deployments. Use NFS over a Cinder-backed volume for shared workspace; generate MPI hostfiles from instance IPs.

7. **Swift** provides S3-equivalent object storage for data staging between ADA Cloud and HPC batch systems. It is durable storage that persists after stack/VM deletion.

8. **Hybrid workflows**: ADA Cloud complements, not replaces, HPC batch systems. Use cloud for interactive analysis, pre/post-processing, and embarrassingly parallel bursting; use HPC for tightly-coupled MPI workloads requiring InfiniBand.

9. **Troubleshoot systematically**: quota → console log → security group → network port. Always check quotas first when resource creation fails.

---

## 13. References

- **OpenStack Heat Documentation**: https://docs.openstack.org/heat/wallaby/
- **HOT Template Reference**: https://docs.openstack.org/heat/wallaby/template_guide/hot_spec.html
- **Terraform OpenStack Provider**: https://registry.terraform.io/providers/terraform-provider-openstack/openstack/latest/docs
- **OpenStack Swift Documentation**: https://docs.openstack.org/swift/wallaby/
- **CINECA ADA Cloud Documentation**: https://wiki.u-gov.it/confluence/display/SCAIUS/ADA+Cloud
- **OpenStack User Survey 2023**: https://www.openstack.org/user-survey/
- **HashiCorp Terraform Documentation**: https://developer.hashicorp.com/terraform/docs
- Brikman, Y. (2022). *Terraform: Up & Running* (3rd ed.). O'Reilly Media.
- Sefraoui, O., Aissaoui, M., & Eleuldj, M. (2012). OpenStack: Toward an Open-Source Solution for Cloud Computing Infrastructure. *International Journal of Computer Applications*, 55(3).

---

## 14. Glossary

| Term | Definition |
|------|-----------|
| **Cloud-init** | Standard multi-distribution package for cloud instance initialization; processes `user_data` at first boot |
| **DAG** | Directed Acyclic Graph; Heat builds one to determine resource provisioning order |
| **Data Source** | Terraform resource type that reads (but does not manage) existing infrastructure |
| **Drift** | Divergence between declared infrastructure state (template/HCL) and actual state of deployed resources |
| **Heat** | OpenStack orchestration service; accepts HOT templates and manages stack lifecycle |
| **HCL** | HashiCorp Configuration Language; declarative syntax used in Terraform |
| **HOT** | Heat Orchestration Template; YAML format for describing OpenStack stacks |
| **Idempotent** | Property of an operation: applying it multiple times produces the same result as applying it once |
| **NFS** | Network File System; Linux protocol for sharing directories over a network |
| **Packer** | HashiCorp tool for building machine images across multiple platforms from a single configuration |
| **ResourceGroup** | Heat resource type for creating multiple identical resources with a single definition |
| **Stack** | In Heat, the collection of resources created and managed as a unit from a single template |
| **Swift** | OpenStack object storage service (≈ AWS S3) |
| **Terraform State** | JSON file (or remote backend) recording what Terraform has provisioned; maps HCL to real resource IDs |
| **user_data** | Cloud-init script passed to a VM at launch; executed on first boot by the cloud-init agent |
| **Volume-backed instance** | VM whose root disk is a Cinder volume rather than ephemeral storage; persists after VM deletion |

---


*Lecture: OpenStack Advanced - Infrastructure as Code, Images, and HPC Integration*
