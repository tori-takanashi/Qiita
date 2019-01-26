#pragma once
#include <vtkSmartPointer.h>

class vtkDataSet;
class vtkPolyData;
class vtkPoints;
class vtkImplicitPolyDataDistance;
class vtkUnstructuredGrid;

class vtkVoxelMeshGenerator
{
public:
  vtkVoxelMeshGenerator() {};
  ~vtkVoxelMeshGenerator() {};

  void SetInputData(vtkPolyData * in_polyData);
  void Update();
  void SetParameters();
  void GenerateVoxelMesh();
  const bool IsInsideOrOutside(const double * in_points, vtkImplicitPolyDataDistance * in_implicitPolyDataDistance);
  void AddIs3DModelIds(vtkPoints * in_voxelCenters);
  void AddOriginalVoxelId();
  vtkUnstructuredGrid * GetOutput();

  // setter èëÇ´ï˚å√Ç¢Ç©Ç‡ÇæÇØÇ«ãñÇµÇƒÇ≠ÇÍ
  void SetCellDimensions(const int *in_dims) { memcpy_s(cellDims, sizeof(int) * 3, in_dims, sizeof(int) * 3); }
  void SetOffsets(const double *in_offsets) { memcpy_s(offsets, sizeof(double) * 6, in_offsets, sizeof(double) * 6); }

private:
  double *bounds;
  double meshPitches[3];
  double mins[3];

  int cellDims[3];
  double offsets[6];

  vtkSmartPointer<vtkPolyData> polyData;  // input
  vtkSmartPointer<vtkUnstructuredGrid> baseMesh;
  vtkSmartPointer<vtkUnstructuredGrid> voxelMeshForAnalysis; // output
};

