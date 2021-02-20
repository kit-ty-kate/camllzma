module C = Configurator.V1

let ( >>= ) = Option.bind

let () =
  C.main ~name:"camllzma" begin fun c ->
    let default : C.Pkg_config.package_conf =
      { libs = ["-llzma"]
      ; cflags = []
      }
    in
    let conf = C.Pkg_config.get c >>= C.Pkg_config.query ~package:"liblzma" in
    let conf = Option.value ~default conf in
    C.Flags.write_sexp "cflags.sexp" conf.cflags;
    C.Flags.write_sexp "libs.sexp" conf.libs;
  end
