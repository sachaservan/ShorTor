with import <nixpkgs> { };

let
  pythonPackages = python38Packages;
in pkgs.mkShell rec {
  venvDir = "./.venv";
  buildInputs = [
    # Python
    pythonPackages.python
    postgresql  # needed for psycopg2
    # This execute some shell code to initialize a venv in $venvDir before
    # dropping into the shell
    pythonPackages.venvShellHook
    # Linting + development
    nodePackages.pyright
    tor

    vagrant
    qemu
    libvirt

    terraform_0_13
    terraform-providers.openstack
    packer

    openssl
  ];

  # Run this command, only after creating the virtual environment
  postVenvCreation = ''
    unset SOURCE_DATE_EPOCH
    for requirements_file in requirements*.txt; do
      pip install -r $requirements_file
      pip install python-openstackclient
    done
  '';

  postShellHook = ''
    # allow pip to install wheels
    unset SOURCE_DATE_EPOCH
  '';

}
