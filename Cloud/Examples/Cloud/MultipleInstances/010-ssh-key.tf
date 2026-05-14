# Define ssh to config in instance

resource "openstack_compute_keypair_v2" "user_key" {
  name       = "user-key"
  public_key = "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQC5Jj+Ut65HnV7Wfv+EVG5zPjD/NI6eUJdUfPapuzNrVvkVPsgzQieSmJPyypg/hF+Du35PnK+hQ57kodpOu+5VClmGz9DuN0CqqgoBiZqax7uutQo/9DOmfJFm88v7tpA9FG8Hd3rx0GFoJ0MueSyFf8JyR0Lo0HhkoFHcskOMg3fQ8ISdTy3OK099XxrgTafUDoWLwTSHFSWEixGRu8L4QI9MRGNO9GgAfaDxddOFovCEbEGa4BFnTdOgazu6th6Brbb5liUCAMSiRr5q7EQHHEPLvl1iUTLT4WPMo3BR/HmolNyrP0arSWrPZfRg/ZRKAUfaUt/rIIiuadT/Z4Ez gtaffoni@amonra"
}

