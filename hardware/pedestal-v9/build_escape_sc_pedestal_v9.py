"""EscapeSC v9 — flared snow-globe base with Christmas trees.

The cradle ring is now a proper snow-globe pedestal: a flared truncated cone with a
flat foot and a stepped lip at the top, decorated with 8 raised Christmas trees.
Still open all the way through so the NFC tag is never covered.

The pedestal dish is widened to the new base's foot diameter.

Run: blender --background --python build_escape_sc_pedestal_v9.py
"""
import bpy, bmesh, os, struct, math
from mathutils import Vector

OUT = os.path.dirname(os.path.abspath(__file__))

# ---------------------------- globe / ring ----------------------------
GLOBE_BASE_OD = 111.8
BORE_CLEAR    = 0.6
RING_BORE     = GLOBE_BASE_OD + BORE_CLEAR      # 112.4
RING_TOP_OD   = 119.0
RING_BOT_OD   = 130.0
RING_H        = 20.0
FOOT_H        = 3.0                             # straight plinth at the bottom
LIP_H         = 3.0                             # stepped lip at the top
N_TREES       = 8

DISH_CLEAR = 0.6
DISH_D     = RING_BOT_OD + DISH_CLEAR           # 130.6

# ---------------------------- pedestal --------------------------------
TAG_OD = 34.0
DISH_DEPTH, TAG_POCKET_D, ROOF_T = 2.5, 0.4, 0.6
PYR_BASE, PYR_TOP, PYR_H, WALL = 205.0, 160.0, 78.0, 3.2
PLATE_W, PLATE_T = 150.0, 3.5
CAVITY_TOP = 138.0
STRAIGHT_H = 12.0
BORE = 191.6
LID_T = 4.0

TOP_Z   = PYR_H
DISH_Z  = TOP_Z - DISH_DEPTH
TAG_Z   = DISH_Z - TAG_POCKET_D
PLATE_B = TAG_Z - ROOF_T
ANT_Z   = PLATE_B - 1.6

# ------------------------------ helpers -------------------------------
def clean():
    bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)

def col(n):
    c = bpy.data.collections.new(n); bpy.context.scene.collection.children.link(c); return c

def move(o, c):
    for x in list(o.users_collection): x.objects.unlink(o)
    c.objects.link(o); return o

def cube(n, d, loc, c, b=0.0):
    bpy.ops.mesh.primitive_cube_add(location=loc)
    o = bpy.context.object; o.name = n; o.dimensions = d
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if b:
        m = o.modifiers.new('r', 'BEVEL'); m.width = b; m.segments = 3
        bpy.context.view_layer.objects.active = o
        bpy.ops.object.modifier_apply(modifier=m.name)
    return move(o, c)

def cyl(n, r, h, loc, c, v=128):
    bpy.ops.mesh.primitive_cylinder_add(vertices=v, radius=r, depth=h, location=loc)
    o = bpy.context.object; o.name = n; return move(o, c)

def cone(n, r1, r2, h, loc, c, v=128):
    bpy.ops.mesh.primitive_cone_add(vertices=v, radius1=r1, radius2=r2, depth=h, location=loc)
    o = bpy.context.object; o.name = n; return move(o, c)

def box4(n, r1, r2, h, loc, c):
    o = cone(n, r1 * 1.41421356, r2 * 1.41421356, h, loc, c, 4)
    o.rotation_euler[2] = 0.7853981634
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    return o

def cut(o, x):
    bpy.context.view_layer.objects.active = o
    m = o.modifiers.new('c', 'BOOLEAN'); m.operation = 'DIFFERENCE'
    m.solver = 'EXACT'; m.object = x
    bpy.ops.object.modifier_apply(modifier=m.name)
    bpy.data.objects.remove(x, do_unlink=True)
    return o

def merge(o, x):
    bpy.context.view_layer.objects.active = o
    m = o.modifiers.new('u', 'BOOLEAN'); m.operation = 'UNION'
    m.solver = 'EXACT'; m.object = x
    bpy.ops.object.modifier_apply(modifier=m.name)
    bpy.data.objects.remove(x, do_unlink=True)
    return o

def prism(name, verts, faces, c):
    me = bpy.data.meshes.new(name + 'M'); me.from_pydata(verts, [], faces); me.update()
    o = bpy.data.objects.new(name, me); c.objects.link(o)
    bm = bmesh.new(); bm.from_mesh(me)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(me); bm.free()
    return o


def star_prism_radial(zc, ro, depth, a, cone_r_fn):
    """5-point star engraved into a sloped surface, facing radially outward.
    Faces are triangle FANS: a concave 10-gon star triangulates badly and yields
    non-manifold output, which is what broke the first attempt."""
    pts = []
    for i in range(10):
        ang = math.radians(90 + i * 36)
        r = ro if i % 2 == 0 else ro * 0.45
        pts.append((r * math.sin(ang), r * math.cos(ang)))
    R = cone_r_fn(zc)
    ca, sa = math.cos(a), math.sin(a)
    def w(rad, v, z):
        return (rad * ca - v * sa, rad * sa + v * ca, z)
    verts = []
    for rad in (R - depth, R + 3.0):
        verts.append(w(rad, 0.0, zc))                       # fan centre
        for (u, v) in pts:
            verts.append(w(rad, v, zc + u))
    faces = []
    for i in range(10):
        j = (i + 1) % 10
        faces.append([0, 1 + i, 1 + j])                     # inner fan
        faces.append([11, 12 + j, 12 + i])                  # outer fan
        faces.append([1 + i, 12 + i, 12 + j, 1 + j])        # side wall
    return verts, faces

def star_prism_flat(cx, cy, ro, depth, ztop):
    pts = []
    for i in range(10):
        ang = math.radians(90 + i * 36)
        r = ro if i % 2 == 0 else ro * 0.45
        pts.append((cx + r * math.cos(ang), cy + r * math.sin(ang)))
    verts = [(cx, cy, ztop - depth)] + [(x, y, ztop - depth) for x, y in pts]
    verts += [(cx, cy, ztop + 2.0)] + [(x, y, ztop + 2.0) for x, y in pts]
    faces = []
    for i in range(10):
        j = (i + 1) % 10
        faces.append([0, 1 + i, 1 + j])
        faces.append([11, 12 + j, 12 + i])
        faces.append([1 + i, 12 + i, 12 + j, 1 + j])
    return verts, faces

def stl(objs, path):
    dg = bpy.context.evaluated_depsgraph_get(); tris = []
    for o in objs:
        if o.type != 'MESH': continue
        eo = o.evaluated_get(dg); me = eo.to_mesh(); me.calc_loop_triangles()
        for q in me.loop_triangles:
            vs = [eo.matrix_world @ me.vertices[i].co for i in q.vertices]
            n = (vs[1] - vs[0]).cross(vs[2] - vs[0]).normalized()
            tris.append((n, vs))
        eo.to_mesh_clear()
    with open(path, 'wb') as f:
        f.write(b'EscapeSC v8'.ljust(80, b' ')); f.write(struct.pack('<I', len(tris)))
        for n, vs in tris:
            f.write(struct.pack('<3f', *n))
            for v in vs: f.write(struct.pack('<3f', *v))
            f.write(struct.pack('<H', 0))

def mat(name, rgb, rough=0.6):
    m = bpy.data.materials.new(name); m.use_nodes = True
    b = m.node_tree.nodes.get('Principled BSDF')
    b.inputs['Base Color'].default_value = (*rgb, 1.0)
    b.inputs['Roughness'].default_value = rough
    m.diffuse_color = (*rgb, 1.0)
    return m

clean()
sc = bpy.context.scene
sc.unit_settings.system = 'METRIC'; sc.unit_settings.length_unit = 'MILLIMETERS'
sc.render.engine = 'BLENDER_EEVEE'
sc.render.resolution_x = 1400; sc.render.resolution_y = 1000
sc.world.color = (.05, .05, .06)

C_BODY  = col('PRINT_1_BODY')
C_PLATE = col('PRINT_2_TOP_PLATE')
C_LID   = col('PRINT_3_BASE_LID')
C_RING  = col('PRINT_4_CRADLE_RING')

# ============================ CRADLE RING ==============================
# Flared pedestal: plinth, cone, stepped lip. Open through the middle.
ring = cyl('CRADLE_RING', RING_BOT_OD / 2, FOOT_H, (0, 0, FOOT_H / 2), C_RING)
body_cone = cone('cone', RING_BOT_OD / 2, RING_TOP_OD / 2, RING_H - FOOT_H,
                 (0, 0, FOOT_H + (RING_H - FOOT_H) / 2), C_RING)
merge(ring, body_cone)
# stepped lip at the top: a slightly wider collar
lip = cyl('lip', (RING_TOP_OD + 5.0) / 2, LIP_H, (0, 0, RING_H - LIP_H / 2), C_RING)
merge(ring, lip)
# straight bore, all the way through so the tag stays exposed
cut(ring, cyl('bore', RING_BORE / 2, RING_H + 20.0, (0, 0, RING_H / 2), C_RING))
# bevel the lip collar edge
bpy.context.view_layer.objects.active = ring
mb = ring.modifiers.new('bev', 'BEVEL'); mb.width = 0.7; mb.segments = 3
bpy.ops.object.modifier_apply(modifier=mb.name)

# --- Christmas trees (ENGRAVED) -----------------------------------------
# Union of tree prisms kept collapsing to a single blob. Relief is cut instead:
# subtraction has been reliable throughout, and it prints cleaner than proud fins.
TW, TWt, TH = 7.0, 1.8, 11.0     # base width, truncated apex width, height
DEPTH = 1.5
Z0 = 4.5
def cone_r(z):
    return RING_BOT_OD / 2 + (RING_TOP_OD / 2 - RING_BOT_OD / 2) * ((z - FOOT_H) / (RING_H - FOOT_H))
def r_in(z):  return cone_r(z) - DEPTH
def r_out(z): return cone_r(z) + 3.0
verts, faces = [], []
for i in range(N_TREES):
    a = 2 * math.pi * i / N_TREES
    ca, sa = math.cos(a), math.sin(a)
    def pta(r, w, z):
        return (r * ca - w * sa, r * sa + w * ca, z)
    b = len(verts)
    zb, za = Z0, Z0 + TH
    # Truncated apex. A true point produces zero-width slivers in the boolean and
    # leaves open edges, which is what broke the first two attempts.
    verts += [pta(r_in(zb), -TW / 2, zb), pta(r_in(zb), TW / 2, zb),
              pta(r_in(za), TWt / 2, za), pta(r_in(za), -TWt / 2, za)]
    verts += [pta(r_out(zb), -TW / 2, zb), pta(r_out(zb), TW / 2, zb),
              pta(r_out(za), TWt / 2, za), pta(r_out(za), -TWt / 2, za)]
    faces += [[b, b + 1, b + 2, b + 3],
              [b + 4, b + 7, b + 6, b + 5],
              [b, b + 4, b + 5, b + 1],
              [b + 1, b + 5, b + 6, b + 2],
              [b + 2, b + 6, b + 7, b + 3],
              [b + 3, b + 7, b + 4, b]]
cutter = prism('tree_cutter', verts, faces, C_RING)
cut(ring, cutter)

# --- upper band: 8 engraved stars, offset half a pitch from the trees -----
N_STARS = 8
sv, sf = [], []
for i in range(N_STARS):
    a = 2 * math.pi * i / N_STARS + math.pi / N_STARS
    v_, f_ = star_prism_radial(12.4, 3.2, 1.0, a, cone_r)
    b = len(sv); sv.extend(v_)
    for f in f_: sf.append([x + b for x in f])
cut(ring, prism('ring_stars', sv, sf, C_RING))


# ================================= BODY ================================
body = box4('BODY', PYR_BASE / 2, PYR_TOP / 2, PYR_H, (0, 0, PYR_H / 2), C_BODY)
bore = cube('bore', (BORE, BORE, STRAIGHT_H + 10.0), (0, 0, (STRAIGHT_H - 10.0) / 2), C_BODY)
taper = box4('taper', BORE / 2, CAVITY_TOP / 2, PLATE_B - STRAIGHT_H,
             (0, 0, STRAIGHT_H + (PLATE_B - STRAIGHT_H) / 2), C_BODY)
merge(bore, taper)
cut(body, bore)
cut(body, cube('rebate', (PLATE_W, PLATE_W, PLATE_T + 10.0),
               (0, 0, PLATE_B + (PLATE_T + 10.0) / 2), C_BODY))

# ============================== TOP PLATE ==============================
plate = cube('TOP_PLATE', (PLATE_W - 1.0, PLATE_W - 1.0, PLATE_T),
             (0, 0, PLATE_B + PLATE_T / 2), C_PLATE, 0.5)
cut(plate, cyl('dish', DISH_D / 2, DISH_DEPTH + 8.0,
               (0, 0, DISH_Z + (DISH_DEPTH + 8.0) / 2), C_PLATE))
cut(plate, cyl('tag', TAG_OD / 2, TAG_POCKET_D + 8.0,
               (0, 0, TAG_Z + (TAG_POCKET_D + 8.0) / 2), C_PLATE))

# --- engraved pinstripe on the plate rim -------------------------------
# The rim between the dish (r 65.3) and the plate edge (r 74.5) is only 9.2 mm
# wide, so 5-point stars barely register. A double groove reads as deliberate.
for i, r_mid in enumerate((68.6, 72.0)):
    outer = cyl('groove_o%d' % i, (r_mid + 0.45) / 1.0, 1.6, (0, 0, TOP_Z - 0.2), C_PLATE)
    cut(outer, cyl('groove_i%d' % i, (r_mid - 0.45) / 1.0, 1.8, (0, 0, TOP_Z - 0.2), C_PLATE))
    cut(plate, outer)

# =============================== BASE LID ==============================
lid = cube('BASE_LID', (BORE - 1.0, BORE - 1.0, LID_T), (0, 0, LID_T / 2), C_LID, 1.2)
lip2 = cube('lip2', (BORE - 8.0, BORE - 8.0, 8.0), (0, 0, 8.0), C_LID, 0.8)
cut(lip2, cube('lip_in', (BORE - 16.0, BORE - 16.0, 10.0), (0, 0, 8.0), C_LID))
merge(lid, lip2)
cut(lid, cube('notch', (26, 10, 14), (0, -(BORE - 1.0) / 2 + 2, 6), C_LID, 1.5))

# ------------------------------ MATERIALS ------------------------------
for cc, nm, rgb in ((C_BODY, 'PLA_DeepBlue', (0.055, 0.20, 0.42)),
                    (C_PLATE, 'PLA_Amber', (0.72, 0.42, 0.06)),
                    (C_LID, 'PLA_SteelBlue', (0.11, 0.34, 0.62)),
                    (C_RING, 'PLA_ChristmasGreen', (0.09, 0.34, 0.16))):
    m = mat(nm, rgb)
    for o in cc.objects:
        if hasattr(o.data, 'materials'): o.data.materials.append(m)

# -------------------------------- EXPORT -------------------------------
stl(list(C_BODY.objects),  os.path.join(OUT, 'EscapeSC_v9_Body.stl'))
stl(list(C_PLATE.objects), os.path.join(OUT, 'EscapeSC_v9_TopPlate.stl'))
stl(list(C_LID.objects),   os.path.join(OUT, 'EscapeSC_v9_BaseLid.stl'))
stl(list(C_RING.objects),  os.path.join(OUT, 'EscapeSC_v9_CradleRing.stl'))
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, 'EscapeSC_v9.blend'))

bpy.ops.object.light_add(type='AREA', location=(60, -150, 240)); bpy.context.object.data.energy = 2400; bpy.context.object.data.size = 190
bpy.ops.object.light_add(type='AREA', location=(-170, 110, 130)); bpy.context.object.data.energy = 1300; bpy.context.object.data.size = 130

def shoot(name, loc, target, hide_others=False):
    if hide_others:
        for o in bpy.data.objects:
            if o.type == 'MESH' and o.name != 'CRADLE_RING':
                o.hide_render = True
    bpy.ops.object.camera_add(location=loc)
    cam = bpy.context.object
    cam.rotation_euler = (Vector(target) - cam.location).to_track_quat('-Z', 'Y').to_euler()
    sc.camera = cam
    sc.render.filepath = os.path.join(OUT, name)
    bpy.ops.render.render(write_still=True)

shoot('EscapeSC_v9_hero.png', (330, -360, 280), (0, 0, 60))
shoot('EscapeSC_v9_ring.png', (185, -215, 135), (0, 0, 9), hide_others=True)

print('V8_DONE')
print(f'  ring: foot OD {RING_BOT_OD}  top OD {RING_TOP_OD}  h {RING_H}  bore {RING_BORE}  trees {N_TREES}')
print(f'  dish {DISH_D}  -> clearance {(DISH_D-RING_BOT_OD)/2:.2f}/side')
print(f'  gap tag->antenna {TAG_Z-ANT_Z:.2f}')