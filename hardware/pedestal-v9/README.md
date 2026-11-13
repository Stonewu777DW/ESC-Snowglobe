# Pedestal v9 — mechanical assets

Includes Body, TopPlate, BaseLid and CradleRing STLs, the editable Blender scene,
original hero reference and generator from the project's mechanical directory.
Geometry assets are unchanged. The generator now writes beside itself for portability.

Open EscapeSC_v9.blend in Blender, or run:

```sh
blender --background --python hardware/pedestal-v9/build_escape_sc_pedestal_v9.py
```

This regenerates/overwrites v9 assets beside the script. Save edits first. It needs
Blender's bpy/bmesh/mathutils: ordinary Python cannot execute it. Syntax-checked
here, not re-rendered or freshly geometry-accepted.

Print one body, top plate and base lid, plus one cradle ring per globe (four for
this playlist). Slice for your own printer/material. Machine-specific G-code
and old iterations are deliberately not shipped.

- Top and bottom are removable; top seats without a positive latch.
- Support/secure the top before inverting for battery access.
- RC522 antenna goes under the top plate; tags belong on globes, not the pedestal.
- Specified tag/antenna gap is 2.2 mm; test the actual print/adhesive.
- Secure Arduino, battery and wires; the model is not proof of a battery holder.
- Final closed-enclosure playback and battery runtime remain unverified.
