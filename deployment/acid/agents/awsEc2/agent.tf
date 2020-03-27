provider "aws" {
  region = "eu-west-1"
}

variable "instance_type" {
  type        = string
  default     = "f1.2xlarge"
  description = "Instance type to use."
}

variable "spot" {
  type        = bool
  default     = true
  description = "Use spot instance."
}

# Configure AMI
locals {
  # Available images
  ami_users = {
    "centos_7"                     = "centos"
    "centos_7_aws_fpga_dev"        = "centos"
    "centos_7_aws_fpga_dev_2017_4" = "centos"
    "centos_7_aws_fpga_dev_2018_2" = "centos"
    "centos_7_aws_fpga_dev_2018_3" = "centos"
    "centos_7_aws_fpga_dev_2019_1" = "centos"
    "centos_7_aws_fpga_dev_2019_2" = "centos"
    "debian_9"                     = "admin"
    "debian_10"                    = "admin"
    "fedora_30"                    = "fedora"
    "fedora_31"                    = "fedora"
    "ubuntu_16_04"                 = "ubuntu"
    "ubuntu_18_04"                 = "ubuntu"
  }
  ami_owners = {
    "centos_7"                     = "679593333241"
    "centos_7_aws_fpga_dev"        = "679593333241"
    "centos_7_aws_fpga_dev_2017_4" = "679593333241"
    "centos_7_aws_fpga_dev_2018_2" = "679593333241"
    "centos_7_aws_fpga_dev_2018_3" = "679593333241"
    "centos_7_aws_fpga_dev_2019_1" = "679593333241"
    "centos_7_aws_fpga_dev_2019_2" = "679593333241"
    "debian_9"                     = "379101102735"
    "debian_10"                    = "136693071363"
    "fedora_30"                    = "125523088429"
    "fedora_31"                    = "125523088429"
    "ubuntu_16_04"                 = "099720109477"
    "ubuntu_18_04"                 = "099720109477"
  }
  ami_names = {
    "centos_7"                     = "CentOS Linux 7 x86_64 HVM EBS ENA *"
    "centos_7_aws_fpga_dev"        = "FPGA Developer AMI - *"
    "centos_7_aws_fpga_dev_2017_4" = "FPGA Developer AMI - 1.4.*"
    "centos_7_aws_fpga_dev_2018_2" = "FPGA Developer AMI - 1.5.*"
    "centos_7_aws_fpga_dev_2018_3" = "FPGA Developer AMI - 1.6.*"
    "centos_7_aws_fpga_dev_2019_1" = "FPGA Developer AMI - 1.7.*"
    "centos_7_aws_fpga_dev_2019_2" = "FPGA Developer AMI - 1.8.*"
    "debian_9"                     = "debian-stretch-hvm-x86_64-gp2-*"
    "debian_10"                    = "debian-10-amd64-*"
    "fedora_30"                    = "Fedora-Cloud-Base-30-*"
    "fedora_31"                    = "Fedora-Cloud-Base-31-*"
    "ubuntu_16_04"                 = "ubuntu/images/ebs-ssd/ubuntu-xenial-16.04-amd64-server-*"
    "ubuntu_18_04"                 = "ubuntu/images/hvm-ssd/ubuntu-bionic-18.04-amd64-server-*"
  }

  # Remote user
  user = local.ami_users[var.image]
}

data "aws_ami" "image" {
  most_recent = true
  owners      = [local.ami_owners[var.image]]
  filter {
    name   = "name"
    values = [local.ami_names[var.image]]
  }
  filter {
    name   = "virtualization-type"
    values = ["hvm"]
  }
  filter {
    name   = "architecture"
    values = ["x86_64"]
  }
}

# Configure security
resource "aws_key_pair" "key_pair" {
  key_name   = local.name
  public_key = local.public_key
}

resource "aws_security_group" "security_group" {
  name = local.name
  dynamic "ingress" {
    for_each = local.firewall_rules
    content {
      from_port        = ingress.value.port
      to_port          = ingress.value.port
      protocol         = ingress.value.protocol
      cidr_blocks      = ingress.value.cidr_blocks
      ipv6_cidr_blocks = ingress.value.ipv6_cidr_blocks
    }
  }
  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
}

# Configure instance
resource "aws_spot_instance_request" "spot_instance" {
  ami                  = data.aws_ami.image.id
  instance_type        = var.instance_type
  iam_instance_profile = "AccelizeAzurePipelineAgent"
  security_groups      = [aws_security_group.security_group.name]
  key_name             = aws_key_pair.key_pair.key_name
  tags = {
    Name = local.name
  }
  root_block_device {
    delete_on_termination = true
  }

  # Spot specific
  count                = var.spot ? 1 : 0
  spot_type            = "one-time"
  wait_for_fulfillment = true
  provisioner "local-exec" {
    # "tags" apply to spot instance request and needs to be applied to instance
    # https://github.com/terraform-providers/terraform-provider-aws/issues/32
    command = <<-EOF
    aws ec2 create-tags --region eu-west-1 \
    --resources ${self.spot_instance_id} --tags Key=Name,Value="${local.name}" \
    EOF
  }

  # Instance configuration
  provisioner "remote-exec" {
    # Wait until instance is ready
    inline = ["cd"]
    connection {
      host        = self.public_ip
      type        = "ssh"
      user        = local.user
      private_key = local.private_key
    }
  }
  provisioner "local-exec" {
    # Configure using Ansible
    command = "${local.ansible} -i '${self.public_ip},'"
  }
}

resource "aws_instance" "instance" {
  ami                  = data.aws_ami.image.id
  instance_type        = var.instance_type
  iam_instance_profile = "AccelizeAzurePipelineAgent"
  security_groups      = [aws_security_group.security_group.name]
  key_name             = aws_key_pair.key_pair.key_name
  tags = {
    Name = local.name
  }
  root_block_device {
    delete_on_termination = true
  }

  # On-demand specific
  count                                = var.spot ? 0 : 1
  instance_initiated_shutdown_behavior = "terminate"

  # Instance configuration
  provisioner "remote-exec" {
    # Wait until instance is ready
    inline = ["cd"]
    connection {
      host        = self.public_ip
      type        = "ssh"
      user        = local.user
      private_key = local.private_key
    }
  }
  provisioner "local-exec" {
    # Configure using Ansible
    command = "${local.ansible} -i '${self.public_ip},'"
  }
}
