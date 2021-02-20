(lang dune 2.7)
(name camllzma)
(version 0.1.0~)

(formatting disabled)
(generate_opam_files true)

(license MIT)
(maintainers "Kate <kit.ty.kate@disroot.org>")
(authors "Kate <kit.ty.kate@disroot.org>")
(source (github kit-ty-kate/camllzma))

(package
 (name camllzma)
 (synopsis "Bindings for libLZMA")
 (tags (compression xz lzma))
 (depends
  (ocaml (>= 4.08))
  dune
  (dune-configurator (and :build (>= 2.7)))
  (conf-pkg-config :build)
  (conf-liblzma :build)))
