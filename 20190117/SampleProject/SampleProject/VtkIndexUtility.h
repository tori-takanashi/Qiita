#pragma once

namespace VtkIndexUtility
{
  template<typename T>
  const T ConvertXYZToVtkCellId(const T & in_x, const T & in_y, const T & in_z, const int in_cellDims[3])
  {
    return in_x + in_y * in_cellDims[0] + in_z * in_cellDims[0] * in_cellDims[1];
  }

  // voxelId -> xyzId
  // 0 <= xId <= CellDimX - 1, 
  // 0 <= yId <= CellDimX * CellDimY - 1, 
  // 0 <= zId <= CellDimX * CellDimY * CellDimZ - 1 ‚Æ‚È‚éB
  template<typename T>
  const T * ConvertVtkCellIdToXYZ(const T & in_voxelId, const int in_cellDims[3])
  {
    vtkIdType xyzId[3]; // 0:x, 1:y, 2:z

    // zId = vId / (CellDimX * CellDimY)
    xyzId[2] = in_voxelId / (in_cellDims[0] * in_cellDims[1]);

    // xId + yId * CellDimX = vId % (CellDimX * CellDimY)
    // yId = vId % (CellDimX * CellDimY) / CellDimX
    xyzId[1] = (in_voxelId % (in_cellDims[0] * in_cellDims[1])) / in_cellDims[0];

    // xId = (xId + yId * CellDimX) % CellDimX
    xyzId[0] = (in_voxelId % (in_cellDims[0] * in_cellDims[1]) % in_cellDims[0]);

    return xyzId;
  }

  // 0:x-1, 1:x+1, 2:y-1, 3:y+1, 4:z-1, 5:z+1
  template<typename T>
  const T * GetPeripheralXYZs(const T & in_x, const T & in_y, const T & in_z, const int in_cellDims[3])
  {
    const T peripheralXYZs[6] =
    {
       ConvertXYZToVtkCellId(in_x - 1, in_y, in_z, in_cellDims),
       ConvertXYZToVtkCellId(in_x + 1, in_y, in_z, in_cellDims),
       ConvertXYZToVtkCellId(in_x, in_y - 1, in_z, in_cellDims),
       ConvertXYZToVtkCellId(in_x, in_y + 1, in_z, in_cellDims),
       ConvertXYZToVtkCellId(in_x, in_y, in_z - 1, in_cellDims),
       ConvertXYZToVtkCellId(in_x, in_y, in_z + 1, in_cellDims)
    };
    return peripheralXYZs;
  }

  template<typename T>
  const bool IsInsideField(const T &in_x, const T &in_y, const T &in_z, const int in_cellDims[3])
  {
    const bool isInsideX = (0 < in_x || in_x < in_cellDims[0] - 1);
    const bool isInsideY = (0 < in_y || in_y < in_cellDims[1] - 1);
    const bool isInsideZ = (0 < in_z || in_z < in_cellDims[2] - 1);

    if (!isInsideX || !isInsideY || !isInsideZ)
      return false;

    return true;
  }

}