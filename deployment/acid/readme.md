# ACID: Accelize Continuous Integration & Delivery

Some Accelize tools used for CI/CD and DevOps.

## Content

A quick content description.

* `roles`: Ansible roles.
    * `azure_pipeline_agent`: Install Azure pipeline agent.
* `agents`: Hosted Azure Pipeline agent configuration.
    * `start.yml`: Azure Pipeline job template to start agent.
    * `stop.yml`: Azure Pipeline job template to stop agent started with
      `start.yml`.
    * subdirectories: Agents related Terraform configurations.
* `scripts`: Utilities script.
    * `install_packages.py`: Auto detect package manager and install all
      packages from the specified directory.
    * `sign_packages.py`: Sign RPM & DEB packages.
* Python library: `acid` can be imported as Python library that provides some
  functions indented to be used from inside CI pipelines.

`start.yml` require that the `agentManagerToken` variable is set to a Azure 
DevOps Personal access Token that give `read`, `manage` accesses to 
`Agent Pools` and `Deployment group` scopes.
