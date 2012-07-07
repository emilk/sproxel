import sproxel
import struct, zlib, os


plugin_info=dict(
  name='Paint3D import/export'
  )


def register():
  sproxel.register_importer(Paint3dImporter)
  sproxel.register_exporter(Paint3dExporter)


def unregister():
  sproxel.unregister_importer(Paint3dImporter)
  sproxel.unregister_exporter(Paint3dExporter)



def packColorComp(f):
  if f<=0 : return 0
  if f>=1 : return 255
  return int(round(f*255))



class Paint3dImporter(object):
  @staticmethod
  def name(): return 'Paint3D file'

  @staticmethod
  def filter(): return '*.3mp'

  @staticmethod
  def doImport(fn, um, prj, cur_sprite):

    with open(fn, 'rb') as f:
      if f.read(3)!='3MP':
        print 'Not a 3MP file!'
        return False

      version=struct.unpack('<I', f.read(4))[0]
      if version!=1 and version!=2:
        print 'Unsupported 3MP file version '+version
        return False

      sizex, sizey, sizez = struct.unpack('<III', f.read(12))

      if version==2:
        voxelScale=struct.unpack('<fff', f.read(12))
      else:
        voxelScale=(1.0, 1.0, 1.0)

      compression=struct.unpack('<B', f.read(1))[0]

      #print 'file "%s": %dx%dx%d pack=%d' % (fn, sizex, sizey, sizez, compression)

      layer=sproxel.Layer((sizex, sizez, sizey))

      cfmt=struct.Struct('<4B')

      if compression==0:
        data=f.read()
        ofs=0
        for x in xrange(sizex):
          for y in xrange(sizey-1, -1, -1):
            for z in xrange(sizez):
              b, g, r, a = cfmt.unpack_from(data, ofs)
              ofs+=cfmt.size
              layer.set(x, z, y, (r/255.0, g/255.0, b/255.0, a/255.0))

      elif compression==1:
        x = 0
        y = 0
        z = 0
        data=f.read()
        length=len(data)
        ofs=0

        bfmt=struct.Struct('<B')

        while ofs<length:
          counter = bfmt.unpack_from(data, ofs)
          ofs+=1

          b, g, r, a = cfmt.unpack_from(data, ofs)
          ofs+=cfmt.size

          color=(r/255.0, g/255.0, b/255.0, a/255.0)

          for i in xrange(counter):
            layer.set(x, z, sizey-1-y, color)

            z = z + 1
            if z >= sizez:
              z = 0
              y = y + 1
              if y >= sizey:
                y = 0
                x = x + 1

      elif compression==2:
        data=zlib.decompress(f.read(), -15)
        ofs=0
        for x in xrange(sizex):
          for y in xrange(sizey-1, -1, -1):
            for z in xrange(sizez):
              b, g, r, a = cfmt.unpack_from(data, ofs)
              ofs+=cfmt.size
              layer.set(x, z, y, (r/255.0, g/255.0, b/255.0, a/255.0))

      else:
        print 'Unsupported 3MP file compression method '+compression
        return False

      spr=sproxel.Sprite(layer)
      spr.name=os.path.splitext(os.path.basename(fn))[0]
      um.addSprite(prj, -1, spr)

    return True



class Paint3dExporter(object):
  @staticmethod
  def name(): return 'Paint3D file'

  @staticmethod
  def filter(): return '*.3mp'

  @staticmethod
  def doExport(fn, prj, cur_sprite):
    if not fn.lower().endswith('.3mp') : fn+='.3mp'

    bounds=cur_sprite.bounds
    sizex=bounds[1][0]-bounds[0][0]+1
    sizey=bounds[1][1]-bounds[0][1]+1
    sizez=bounds[1][2]-bounds[0][2]+1

    compression=2 # set to 0 to disable compression

    with open(fn, 'wb') as f:
      f.write('3MP')
      f.write(struct.pack('<I', 2)) # version
      f.write(struct.pack('<III', sizex, sizez, sizey))
      f.write(struct.pack('<fff', 1.0, 1.0, 1.0)) # voxel scale
      f.write(struct.pack('<B', compression)) # compression method

      cfmt=struct.Struct('<4B')

      buf=bytearray(sizex*sizey*sizez*cfmt.size)
      ofs=0
      for x in xrange(sizex):
        for z in xrange(sizez-1, -1, -1):
          for y in xrange(sizey):
            r, g, b, a=cur_sprite.getColor(
              x+bounds[0][0],
              y+bounds[0][1],
              z+bounds[0][2])

            cfmt.pack_into(buf, ofs,
              packColorComp(b),
              packColorComp(g),
              packColorComp(r),
              packColorComp(a))
            ofs+=cfmt.size

      if compression==2:
        packed=zlib.compress(str(buf))
        f.write(packed[2:-4])
      else:
        f.write(buf)

    return True
