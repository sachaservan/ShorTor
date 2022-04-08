# A lot of this is pretty specific to the CSAIL OpenStack deployment.

# NOTE: needs `openssl` on the path due to https://github.com/hashicorp/packer/issues/2526

variable "csail_username" {
  type        = string
  description = "CSAIL username (ex. `zjn`)"
}
variable "csail_password" {
  type        = string
  sensitive   = true
  description = "CSAIL password (ex. `letmein`). Use $PKR_VAR_csail_password for this."
}

source "openstack" "common" {
  source_image_filter {
    filters {
      name = "Ubuntu-20.04LTS-amd64"
    }
    most_recent = true
  }
  ssh_username = "ubuntu"
  # Workaround for https://tig.csail.mit.edu/network-wireless/ssh/
  # (we can't SSH directly)
  ssh_bastion_username = var.csail_username
  ssh_bastion_host     = "jump.csail.mit.edu"
  # Workaround for https://github.com/hashicorp/packer/issues/10364
  # (no Kerberos support in Packer)
  ssh_bastion_password = var.csail_password
  # CSAIL OpenStack is slow these days
  ssh_timeout = "20m"

  security_groups = ["allow_ssh_permanent"] # NOTE: needs to be created before running Packer
}

# a build block invokes sources and runs provisioning steps on them.
build {
  source "openstack.common" {
    name       = "redis"
    image_name = "shortor-redis"
    flavor     = "s1.1core"
  }

  source "openstack.common" {
    name       = "postgres"
    image_name = "shortor-postgres"
    flavor     = "s1.1core"
  }

  source "openstack.common" {
    name       = "observer"
    image_name = "shortor-observer"
    flavor     = "s1.1core"
  }

  source "openstack.common" {
    name       = "consensus_taker"
    image_name = "shortor-consensus-taker"
    flavor     = "s1.1core"
  }

  source "openstack.common" {
    name       = "throttle"
    image_name = "shortor-throttle"
    flavor     = "s1.1core"
  }

  source "openstack.common" {
    name       = "chutney"
    image_name = "shortor-chutney"
    flavor     = "s1.1core"
  }

  # Common setup.
  provisioner "shell" {
    inline = [
      # We have to fight something (Puppet?) for a lock on /var/lib/apt/lists/
      "sudo systemctl stop unattended-upgrades.service",
      "cloud-init status --wait",
      "while ! sudo apt-get update; do echo 'Failure to update packages'; sleep 1; done",
      "sudo apt-get upgrade -y",  # TODO: do this for prod; it takes so long for dev
      "sudo cloud-init clean"
    ]
  }

  provisioner "file" {
    source = "requirements.txt"
    destination = "/tmp/requirements.txt"
  }

  provisioner "file" {
    source = "latencies"
    destination = "/tmp/latencies"
  }

  provisioner "file" {
    source = "ting"
    destination = "/tmp/ting"
  }

  provisioner "file" {
    source = "/dev/null"
    destination = "/tmp/empty"
    override = {
      consensus_taker = {
        source = "GeoLite2-City.mmdb"
        destination = "/tmp/GeoLite2-City.mmdb"
      }
    }
  }

  provisioner "file" {
    source = "/dev/null"
    destination = "/tmp/empty"
    override = {
      consensus_taker = {
        source = "GeoLite2-ASN.mmdb"
        destination = "/tmp/GeoLite2-ASN.mmdb"
      }
    }
  }

  provisioner "shell" {
    scripts = ["/dev/null"]
    # Run as root
    execute_command = "chmod +x {{ .Path }}; {{ .Vars }} sudo {{ .Path}}"
    override = {
      "redis" = {
        scripts = ["devops/install_redis.sh"]
      }
      "postgres" = {
        scripts = [
          "devops/install_shortor.sh",
          "devops/install_postgres.sh"
        ]
      }
      "observer" = {
        scripts = [
          "devops/install_shortor.sh",
          "devops/install_observer.sh",
        ]
      }
      "consensus_taker" = {
        scripts = [
          "devops/install_shortor.sh",
          "devops/install_consensus_taker.sh",
        ]
      }
      "throttle" = {
        scripts = [
          "devops/install_throttle.sh",
        ]
      }
      "chutney" = {
        scripts = [
          "devops/install_chutney.sh",
        ]
      }
    }
  }
}

