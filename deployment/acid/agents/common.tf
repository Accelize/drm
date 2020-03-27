# Common Terraform configuration

variable "name" {
  type        = string
  description = "Agent name to use."
}

variable "playbook" {
  type        = string
  default     = "playbook.yml"
  description = "Ansible playbook to use."
}

variable "image" {
  type        = string
  default     = "centos_7"
  description = "Image to use."
}

# Resources name

locals {
  name = "AzurePipeline${var.name}"
}

# Ansible command

locals {
  # Ansible-playbook CLI
  ansible = <<-EOF
    ANSIBLE_SSH_ARGS="-o ControlMaster=auto -o ControlPersist=60s" \
    ANSIBLE_PIPELINING="True" ANSIBLE_HOST_KEY_CHECKING="False" \
    ANSIBLE_FORCE_COLOR="True" ANSIBLE_NOCOLOR="False" \
    ansible-playbook ${var.playbook} -u ${local.user} \
    --private-key '${local_file.ssh_key_file.filename}' \
  EOF
}

# Firewall configuration

data "http" "public_ip" {
  url = "https://api.ipify.org"
}

locals {
  firewall_rules = [
    # Allow SSH from agent running ANsible
    { port             = 22,
      protocol         = "tcp",
      cidr_blocks      = ["${chomp(data.http.public_ip.body)}/32"]
      ipv6_cidr_blocks = null
    }
  ]
}

# SSH Key configuration

resource "tls_private_key" "ssh_key" {
  algorithm = "RSA"
  rsa_bits  = 4096
}

resource "local_file" "ssh_key_file" {
  content  = tls_private_key.ssh_key.private_key_pem
  filename = "${path.module}/ssh_private.pem"
  provisioner "local-exec" {
    command = "chmod 600 ${self.filename}"
  }
}

locals {
  private_key = tls_private_key.ssh_key.private_key_pem
  public_key  = tls_private_key.ssh_key.public_key_openssh
}
