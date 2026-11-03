# Preserved tile world

This is the previous 2.5D tile/house implementation. It is not used by the forest scene.
The active orthographic camera remains in `Camera`, independently of this archive.

Configure with `-DLETSCAMP_BUILD_LEGACY=ON` to compile-check the preserved code.
Running it requires restoring its original map, atlas and house textures, and adding
an explicit scene integration. Those assets are not present in the current project.
The original shader paths were assets/shaders/{basic,building,terrain}.*.
