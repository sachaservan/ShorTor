terraform {
  backend "remote" {
    hostname     = "app.terraform.io"
    organization = "mit-hornet"

    workspaces {
      name = "shortor"
    }
  }
}

variable "concurrent_tor_circuits" {
  type    = number
  default = 10
}

variable "measurements_per_circuit" {
  type = number
  # At 10 samples, we have 95.5% of circuits within 5% or 5ms of the "true
  # minimum" (estimated as the true minimum of 100 samples) and 97.5% of
  # circuits within 5% or 10ms of the "true minimum".
  #
  # See num-measurements.sql (this repo) and num-measurements.gz (Google Drive).
  default = 10
}

variable "max_queue_length" {
  type    = number
  default = 6000
}

variable "max_attempts" {
  type        = number
  description = "Only try to measure a given pair of relays this many times."
  default     = 15
}

variable "number_batches" {
  type        = number
  description = "1 for only CSAIL; 2 for only CSAIL+Ting; -1 for all; probably those are the only useful values."
  default     = -1
}

variable "blacklist" {
  type        = string
  default     = "022A5535F42B1A9F9AA755C4EAB5F36FEF9781D8 CAD1EBF9DCC3E431B96B4293CD41FC4CF1441EF0 336945EDD02FBD58415E8B7666D7F63913770E7E 2AC788BDC15CE88E4F879E75E11A388ED6DC8AA8"
  description = "Space-separated fingerprints of relays to ignore."
}

variable "batch_size" {
  type        = number
  description = <<-EOF
    Our strategy is to measure subsets of relays at a time so that we always
    have a mostly-complete graph. This controls how many additional measurements
    the next subset will have (it's measured in *pairs*, not relays).

      batch_size = (desired batch duration) * (measurement rate)
  EOF
  default     = 250000
}

variable "observer_vms" {
  type    = number
  default = 2
}

variable "real_tor" {
  type    = bool
  default = true
}

resource "random_password" "redis" {
  length  = 24
  special = false
}


provider "openstack" {}

resource openstack_compute_secgroup_v2 "allow_ssh" {
  name        = "allow_ssh"
  description = "allow SSH from whole internet (must use CSAIL jump host)"

  rule {
    from_port   = 22
    to_port     = 22
    ip_protocol = "tcp"
    cidr        = "0.0.0.0/0"
  }
}

resource openstack_compute_secgroup_v2 "allow_ping" {
  name        = "allow_ping"
  description = "allow ping from whole internet"

  rule {
    from_port   = -1
    to_port     = -1
    ip_protocol = "icmp"
    cidr        = "0.0.0.0/0"
  }

}

resource openstack_compute_secgroup_v2 "allow_postgres_internal" {
  name        = "allow_postgres_internal"
  description = "allow postgres from openstack"

  rule {
    from_port   = 5432
    to_port     = 5432
    ip_protocol = "tcp"
    cidr        = local.internal_network
  }
}

resource openstack_compute_secgroup_v2 "allow_redis_internal" {
  name        = "allow_redis_internal"
  description = "allow redis from openstack"

  rule {
    from_port   = 6379
    to_port     = 6379
    ip_protocol = "tcp"
    cidr        = local.internal_network
  }
}

resource openstack_compute_secgroup_v2 "allow_throttle_internal" {
  name        = "allow_throttle_internal"
  description = "allow throttle from openstack"

  rule {
    from_port   = 8888
    to_port     = 8888
    ip_protocol = "tcp"
    cidr        = local.internal_network
  }
}

resource openstack_compute_secgroup_v2 "allow_shortor" {
  name        = "allow_shortor"
  description = "allow shortor measurement traffic from internet"

  # OR port for exit relay
  rule {
    from_port   = 5100
    to_port     = 5101
    ip_protocol = "tcp"
    cidr        = "0.0.0.0/0"
  }

  # DirPort for exit relay
  rule {
    from_port   = 7101
    to_port     = 7101
    ip_protocol = "tcp"
    cidr        = "0.0.0.0/0"
  }
}

resource openstack_compute_secgroup_v2 "allow_shortor_internal" {
  name        = "allow_shortor_internal"
  description = "allow shortor measurement traffic internally"

  # OR port for bridge relay
  rule {
    from_port   = 5100
    to_port     = 5100
    ip_protocol = "tcp"
    cidr        = local.internal_network
  }
  #
  # Echo Server
  rule {
    from_port   = 16000
    to_port     = 17000
    ip_protocol = "tcp"
    cidr        = "0.0.0.0/0"
  }
}

data "openstack_images_image_v2" "redis" {
  name        = "shortor-redis"
  most_recent = true
}

data "openstack_images_image_v2" "throttle" {
  name        = "shortor-throttle"
  most_recent = true
}

data "openstack_images_image_v2" "taker" {
  name        = "shortor-consensus-taker"
  most_recent = true
}

data "openstack_images_image_v2" "observer" {
  name        = "shortor-observer"
  most_recent = true
}

data "openstack_images_image_v2" "postgres" {
  name        = "shortor-postgres"
  most_recent = true
}

locals {
  internal_network = "128.52.128.0/18"

  users_config = {
    groups = ["csg"]
    users = [
      "default",
      {
        name   = "zjn"
        sudo   = "ALL=(ALL) NOPASSWD:ALL"
        groups = ["users", "csg", "systemd-journal"]
        ssh_authorized_keys = [
          "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIMMWcO29f3zUloY5RTJmMMZqQfpMeORbbc+QRRhzHvLE zjn@zjn-x1prime",
          "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIMejdc5AtYy+a4zqlqF1Fj8VdB6VtfIlYvrWELtMh6Bp zjn@zjn-home",
          "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIPlqyM3crc/o8DMR5/yhoZ3Kmm9RzlmCw3gOMv6vGJbW zjn@zjn-workstation",
          "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIHA2+s16j8CHT54sw3eenPv48zg1gHzSabsRhkEt87Ss ben@weintraub.xyz",
          "ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAB3T5rlK6XbAa0nSzxtuyhakXD65nhlo1TkFXXTGz0ZnDAp2oyV5n6zFnwWzmTysf5vt529wVYK2H0dOP+6fI9zHgD/0q32KPYPhxCV6b1QMfQyd8RLNHmqmAch/VcNxqXl3AKXPn8Fbr0j57tsTd3u+eqa9ujN8d+Ihjj3THKFAW/JcQ== kyle@ambiguity",
          "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABgQC26mBILmG9bFMAtheO0JIUP0tubZHsZKGk3Lb3LbWR8zXg5i2t0mSGCS/Uqy+ui0A7oLfqQ0ZkwR//RQPR7us0w5C7iK5w6QdNDitrrkZoCjjEJ1lQBMkyRoy8Fbba8gAGj55IJ/T13JAr+C13LDEFCmwaf4kP2xgASh6ZlZS8ghK6yGhE9aA/lB0ovNe/qgcudigxwiUeQtwMLRD5/aGMXzAKpnujFYt5YmQD8O7F4RFAzr9IeGkPaqcbXfP/GQz4FuQcEWYjoXDNCai7rEsLI6nwvXmPhxZrBB8R4/L3YZiZR6XxQj9xaRiszkUZDmkzZumcTqtYcK6e8IRS98RZEseI6Vg8VBL9Ae9q6hX/r1sk7rLPAevAlhx0eM7fXI8oWPn8YhSQNEBSKN/+9Ene3E3+w/xbRUjBm6SATmdcOa6RyIr6mAQWhQ5XA/gyvTExNixaD2xqlU1dhYTKcTb3+p08uRanzJcsK5H16zPg512Im0IHYIvCaaCj2EkBvyc= Sacha@31-35-54.wireless.csail.mit.edu"
        ],
        shell = "/bin/bash"
      }
    ]

    runcmd = [
      "groupmod -g 16807 csg",
      "id -u postgres && usermod postgres -g csg",
      "id -u shortor && usermod shortor -g csg"
    ]
  }

  redis_user_data = format("#cloud-config\n%s", yamlencode(merge(local.users_config, {
    write_files = [
      {
        content     = <<-EOF
          requirepass ${random_password.redis.result}
        EOF
        owner       = "redis:redis",
        path        = "/etc/redis/runtime.conf",
        permissions = "0644"
  }] })))

  postgres_user_data = format("#cloud-config\n%s", yamlencode(merge(local.users_config, {
    write_files = [
      {
        content     = "data_directory = '/data/shortor'\n",
        owner       = "postgres:postgres",
        path        = "/etc/postgresql/12/main/conf.d/data_dir.conf",
        permissions = "0644"
      },
      {
        content     = <<-EOF
          PG_BACKUP_DIR=${var.real_tor ? "/mnt/db" : "/mnt/db-test"}
        EOF
        owner       = "postgres:postgres",
        path        = "/opt/shortor/shortor.conf",
        permissions = "0644"
      }
    ],
    mounts = [
      ["nfs-prod-14.csail.mit.edu:/export/csg/tor", "/mnt/", "nfs", "rw,sec=sys,vers=3"]
    ],
    runcmd = concat(local.users_config.runcmd, [
      "mkfs.ext4 /dev/vdb", # do this unconditionally on first boot
      "mkdir -p /data",
      "echo '/dev/vdb /data ext4 defaults 0 0' >> /etc/fstab",
      "mount /data",
      "chown postgres:csg /data",
      "mkdir -p /etc/postgresql/12/main",
      "chown -R postgres:postgres /etc/postgresql",
      "sudo -u postgres cp /mnt/keys/tls/ca.crt /etc/postgresql/12/main/server.ca",
      "sudo -u postgres cp /mnt/keys/tls/postgres.crt /etc/postgresql/12/main/server.crt",
      "sudo -u postgres cp /mnt/keys/tls/postgres.key /etc/postgresql/12/main/server.key",
      "bash -c 'chmod 600 /etc/postgresql/12/main/server.{ca,crt,key}'",
      "sudo -u postgres /usr/lib/postgresql/12/bin/initdb -D /data/shortor",
      # Really should do this with After= but we get a cycle:
      #   multi-user.target -> postgresql.service
      #   postgresql.service -> cloud-final.service  (us)
      #   cloud-final.service -> multi-user.target
      # Could resolve but why bother?
      "systemd restart postgresql@12-main.service",
    ])
  })))

  observer_user_data = format("#cloud-config\n%s", yamlencode(merge(local.users_config, {
    write_files = [
      {
        content     = <<-EOF
          CELERY_BROKER_URL=redis://:${random_password.redis.result}@${openstack_compute_instance_v2.redis.access_ip_v4}:6379/0
          SHORTOR_DB_URL="postgresql://shortor@localhost:6432/shortor"
          SHORTOR_THROTTLE_URL="http://${openstack_compute_instance_v2.throttle.access_ip_v4}:8888"
          SHORTOR_SOCKS_PORT=9102
        EOF
        owner       = "shortor:users",
        path        = "/opt/shortor/shortor.conf",
        permissions = "0644"
      },
      {
        content = templatefile("torrc-runtime.prod", {
          real_tor    = var.real_tor,
          authorities = []
        }),
        owner       = "debian-tor:users",
        path        = "/etc/tor/torrc.runtime",
        permissions = "0644"
      },
      { # TODO remove me
        content     = "",
        owner       = "root:root",
        path        = "/etc/tor/instances/exit/torrc.runtime",
        permissions = "0644"
      },
      {
        content     = "shortor = dbname=shortor host=${openstack_compute_instance_v2.postgres.access_ip_v4} port=5432 user=shortor",
        owner       = "postgres:postgres",
        path        = "/etc/pgbouncer/runtime.ini",
        permissions = "0644"
      }
    ],
    runcmd = concat(local.users_config.runcmd, [
      "echo \"ExitPolicy accept $(hostname -I | cut -d' ' -f 1):16000-17000\" > /etc/tor/exitpolicy.torrc",
      "echo \"ExitPolicy reject *:*\"  >> /etc/tor/exitpolicy.torrc",
      "mkdir -p /mnt/keys",
      "sudo -u shortor cp /mnt/keys/tls/ca.crt /opt/shortor/shortor.ca",
      "sudo -u shortor cp /mnt/keys/tls/observer.crt /opt/shortor/shortor.crt",
      "sudo -u shortor cp /mnt/keys/tls/observer.key /opt/shortor/shortor.key",
      "chmod 0600 /opt/shortor/shortor.key",
      "chown postgres /opt/shortor/{shortor.ca,shortor.crt,shortor.key}",
      "usermod _tor-bridge -g csg",
      "usermod _tor-exit -g csg",
      "mkdir -p /var/lib/tor-instances/exit/data",
      "chown -R _tor-exit /var/lib/tor-instances/exit",
      "rm -rf /var/lib/tor-instances/exit/data/keys",
      "sudo -u _tor-exit cp -r /mnt/keys/tor/exitREPLACEME /var/lib/tor-instances/exit/data/keys"
    ]),
    mounts = [
      ["nfs-prod-14.csail.mit.edu:/export/csg/tor", "/mnt/", "nfs", "rw,sec=sys,vers=3"]
    ]
  })))

  consensus_taker_user_data = format("#cloud-config\n%s", yamlencode(merge(local.users_config, {
    write_files = [
      {
        content = <<-EOF
          CELERY_BROKER_URL=redis://:${random_password.redis.result}@${openstack_compute_instance_v2.redis.access_ip_v4}:6379/0
          SHORTOR_CONSENSUS_CACHE_DIR=${var.real_tor ? "/mnt/consensus-cache" : "/mnt/consensus-cache-test"}
          SHORTOR_NUM_MEASUREMENTS=${var.measurements_per_circuit}
          SHORTOR_NUMBER_BATCHES="${var.number_batches != -1 ? var.number_batches : ""}"
          SHORTOR_RELAY_BLACKLIST="${var.blacklist}"
          SHORTOR_MAX_QUEUE_LENGTH=${var.max_queue_length}
          SHORTOR_BATCH_SIZE=${var.batch_size}
          SHORTOR_DB_URL="postgresql://shortor@${openstack_compute_instance_v2.postgres.access_ip_v4}/shortor?sslmode=verify-ca&sslkey=/opt/shortor/shortor.key&sslrootcert=/opt/shortor/shortor.ca&sslcert=/opt/shortor/shortor.crt"
          SHORTOR_GEOIP_CITY=/opt/geo/GeoLite2-City.mmdb
          SHORTOR_GEOIP_ASN=/opt/geo/GeoLite2-ASN.mmdb
          SHORTOR_CONSENSUS_TOR=1
        EOF
        owner   = "shortor:users",
        path    = "/opt/shortor/shortor.conf",
        permissions : "0644"
      }
    ]
    mounts = [
      ["nfs-prod-14.csail.mit.edu:/export/csg/tor", "/mnt/", "nfs", "rw,sec=sys,vers=3"]
    ],
    runcmd = concat(local.users_config.runcmd, [
      "sudo -u shortor cp /mnt/keys/tls/ca.crt /opt/shortor/shortor.ca",
      "sudo -u shortor cp /mnt/keys/tls/observer.crt /opt/shortor/shortor.crt",
      "sudo -u shortor cp /mnt/keys/tls/observer.key /opt/shortor/shortor.key",
      "chmod 0600 /opt/shortor/shortor.key",
    ])
  })))

  throttle_user_data = format("#cloud-config\n%s", yamlencode(merge(local.users_config, {
    write_files = [
      {
        content     = <<-EOF
          [semaphores]
          TOR_CIRCUITS=${var.concurrent_tor_circuits}
        EOF
        owner       = "throttle:users"
        path        = "/etc/throttle.toml"
        permissions = "0644"
      }
    ]
  })))
}

resource "openstack_compute_instance_v2" "redis" {
  name        = "redis"
  image_id    = data.openstack_images_image_v2.redis.id
  flavor_name = "xl.1core"
  security_groups = [
    openstack_compute_secgroup_v2.allow_ssh.name,
    openstack_compute_secgroup_v2.allow_ping.name,
    openstack_compute_secgroup_v2.allow_redis_internal.name
  ]
  user_data = local.redis_user_data
}

data "openstack_blockstorage_volume_v3" "db_data" {
  name = "db_data"
}

resource "openstack_compute_instance_v2" "postgres" {
  name        = "postgres"
  image_id    = data.openstack_images_image_v2.postgres.id
  flavor_name = "lg.12core"
  security_groups = [
    openstack_compute_secgroup_v2.allow_ssh.name,
    openstack_compute_secgroup_v2.allow_ping.name,
    openstack_compute_secgroup_v2.allow_postgres_internal.name
  ]
  user_data = local.postgres_user_data
}

resource "openstack_compute_volume_attach_v2" "postgres_data" {
  instance_id = openstack_compute_instance_v2.postgres.id
  volume_id   = data.openstack_blockstorage_volume_v3.db_data.id
  device      = "/dev/vdb"
}


resource "openstack_compute_instance_v2" "observer" {
  count       = var.observer_vms
  name        = "observer${count.index}"
  image_id    = data.openstack_images_image_v2.observer.id
  flavor_name = "lg.4core"
  security_groups = [
    openstack_compute_secgroup_v2.allow_ssh.name,
    openstack_compute_secgroup_v2.allow_ping.name,
    openstack_compute_secgroup_v2.allow_shortor.name,
    openstack_compute_secgroup_v2.allow_shortor_internal.name,
  ]
  user_data = replace(local.observer_user_data, "REPLACEME", count.index)
}

resource "openstack_compute_instance_v2" "taker" {
  name        = "taker"
  image_id    = data.openstack_images_image_v2.taker.id
  flavor_name = "lg.4core"
  security_groups = [
    openstack_compute_secgroup_v2.allow_ssh.name,
    openstack_compute_secgroup_v2.allow_ping.name,
  ]
  user_data = local.consensus_taker_user_data
}

resource "openstack_compute_instance_v2" "throttle" {
  name        = "throttle"
  image_id    = data.openstack_images_image_v2.throttle.id
  flavor_name = "s1.1core"
  security_groups = [
    openstack_compute_secgroup_v2.allow_ssh.name,
    openstack_compute_secgroup_v2.allow_ping.name,
    openstack_compute_secgroup_v2.allow_throttle_internal.name,
  ]
  user_data = local.throttle_user_data
}
