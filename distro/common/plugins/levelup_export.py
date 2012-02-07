import sproxel
import struct
import zlib


plugin_info=dict(
  name='LEVEL UP! Export Plugin'
  )


def register():
  sproxel.register_exporter(TilesExporter)


def unregister():
  sproxel.unregister_exporter(TilesExporter)


class TilesExporter(object):
  @staticmethod
  def name(): return 'LEVEL UP! tiles'

  @staticmethod
  def filter(): return '*.bin'

  @staticmethod
  def doExport(fn, prj, cur_sprite):
    if not fn.lower().endswith('.bin') : fn+='.bin'
    ok=True

    tfmt=struct.Struct('<3I II')
    fmt32=struct.Struct('<I')

    sprites=prj.sprites
    nameOfs=[]
    dataOfs=[]
    buf=bytearray()

    # fill tile structs
    for spr in sprites:
      buf+=tfmt.size*'\0'

    # write names as C strings and remember offsets
    for spr in sprites:
      nameOfs.append(len(buf))
      buf+=str(spr.name)+'\0'

    # write tile voxel data
    for spr in sprites:
      dataOfs.append(len(buf))

      l=spr.bakeLayers()
      if l.dataType!='IND':
        print 'sprite', spr.name, 'is RGB'
        ok=False
        l=sproxel.Layer()
        l.palette=prj.mainPalette
        l.resize(spr.bounds)

      b=spr.bounds
      for z in range(b[1][2], b[0][2]-1, -1):
        for y in range(b[0][1], b[1][1]+1):
          for x in range(b[0][0], b[1][0]+1):
            buf+=chr(l.getIndex(x, y, z))

    # pack tile structs with actual data
    for i, spr in enumerate(sprites):
      b=spr.bounds
      tfmt.pack_into(buf, i*tfmt.size,
        b[1][0]-b[0][0]+1,
        b[1][1]-b[0][1]+1,
        b[1][2]-b[0][2]+1,
        dataOfs[i],
        nameOfs[i])

    # compress and save to file
    with open(fn, 'wb') as f:
      f.write(fmt32.pack(0x12020800))
      f.write(fmt32.pack(len(sprites)))
      f.write(fmt32.pack(len(buf)))
      packed=zlib.compress(str(buf), 9)
      f.write(fmt32.pack(len(packed)))
      f.write(packed)

    return ok
