#pragma once
#include <vtkSmartPointer.h>

class vtkIdTypeArray;
class vtkUnstructuredGrid;

enum AnalyticProoerty
{
  Analysis,
  NonAnalysis,
  WallX,
  WallY,
  WallZ,
  Surface3DModel
};

class vtkSettingAnalyticProperties
{
public:
  vtkSettingAnalyticProperties() {};
  ~vtkSettingAnalyticProperties() {};

  void SetCellDimensions(const int *in_dims) { memcpy_s(cellDims, sizeof(int) * 3, in_dims, sizeof(int) * 3); }
  void SetInputData(vtkUnstructuredGrid * in_voxelMesh);
  void Update();
  vtkUnstructuredGrid * GetOutput();

private:
  void SetProperties();
  void SetOnePropertyOfAllRegion(AnalyticProoerty in_prop);
  void SetPropertiesOf3DModel();
  void SetPropertiesOfWalls();

  int cellDims[3];

  vtkSmartPointer<vtkUnstructuredGrid> voxelMeshForAnalysis;
  vtkSmartPointer<vtkIdTypeArray> analyticProperties;

};

