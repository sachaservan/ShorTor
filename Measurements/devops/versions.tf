terraform {
  required_providers {
    openstack = {
      source = "terraform-provider-openstack/openstack"
    }
    random = {
      source = "hashicorp/random"
    }
  }
  required_version = ">= 0.13"
}
