import bpy

for part in ['airframe', 'left_flap', 'right_flap', 'left_aileron', 'right_aileron', 'left_elevator', 'right_elevator', 'rudder']:
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_by_type(type='MESH')
    bpy.ops.object.delete(use_global=False)

    print('importing', part)
    bpy.ops.import_mesh.stl(filepath="./stl/{:s}.stl".format(part))

    print('subdividing')
    bpy.ops.object.select_all(action='DESELECT')
    print(list(bpy.data.objects))

    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_by_type(type='MESH')
    bpy.ops.object.editmode_toggle()
    bpy.ops.mesh.subdivide()
    bpy.ops.mesh.subdivide()
    print('exporting')
    bpy.ops.export_mesh.stl(filepath="./stl/{:s}.stl".format(part))

print('exporting complete')
bpy.ops.wm.quit_blender()
