#include "vtkSettingAnalyticProperties.h"
#include "vtkIndexUtility.h"

#include <vtkUnstructuredGrid.h>
#include <vtkDataArray.h>
#include <vtkIdTypeArray.h>
#include <vtkCellData.h>
#include <vtkThreshold.h>
#include <vtkGeometryFilter.h>

void vtkSettingAnalyticProperties::SetInputData(vtkUnstructuredGrid *in_voxelMesh)
{
  this->voxelMeshForAnalysis = vtkSmartPointer<vtkUnstructuredGrid>::New();
  this->voxelMeshForAnalysis->DeepCopy(in_voxelMesh);
}

void vtkSettingAnalyticProperties::Update()
{
  SetProperties();
  this->voxelMeshForAnalysis->GetCellData()->AddArray(analyticProperties);
}

vtkUnstructuredGrid *vtkSettingAnalyticProperties::GetOutput()
{
  return voxelMeshForAnalysis;
}

// 解析場全体の属性を初期化
void vtkSettingAnalyticProperties::SetProperties()
{
  this->analyticProperties = vtkSmartPointer<vtkIdTypeArray>::New();
  analyticProperties->SetName("AnalyticProoerty");
  analyticProperties->SetNumberOfComponents(1);
  analyticProperties->SetNumberOfValues(voxelMeshForAnalysis->GetNumberOfCells());  // 全ボクセル数分だけ配列要素を動的確保

  SetOnePropertyOfAllRegion(Analysis);  // とりあえず、全ボクセルを解析する属性にしておく
  SetPropertiesOf3DModel();   // 3Dモデル用の解析属性を付与
  SetPropertiesOfWalls();   // 壁用の解析属性を付与
}

void vtkSettingAnalyticProperties::SetOnePropertyOfAllRegion(AnalyticProoerty in_prop)
{
  for (vtkIdType vId = 0; vId < voxelMeshForAnalysis->GetNumberOfCells(); vId++)
    this->analyticProperties->SetValue(vId, in_prop);
}

// 3Dモデルに対して解析用の属性を付与します
// 表面には境界条件用の属性、内側には非解析領域の属性を与えるとします
void vtkSettingAnalyticProperties::SetPropertiesOf3DModel()
{
  // 何故かvtkDataArrayでないと抽出できない
  //vtkIdTypeArray *is3DModelIds = vtkIdTypeArray::SafeDownCast(in_grid->GetCellData()->GetArray("Is3DModel"));
  vtkDataArray *is3DModelIds = voxelMeshForAnalysis->GetCellData()->GetArray("Is3DModel");

  // 3Dモデルの全ボクセルに対して解析属性を付与
  for (vtkIdType vId = 0; vId < voxelMeshForAnalysis->GetNumberOfCells(); vId++)
  {
    //if (is3DModelIds->GetValue(vId) == 1)
    if (is3DModelIds->GetTuple1(vId) == 1)
      this->analyticProperties->SetValue(vId, NonAnalysis);
  }

  auto solid3DModel = vtkSmartPointer<vtkThreshold>::New();
  solid3DModel->SetInputData(voxelMeshForAnalysis);
  solid3DModel->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Is3DModel");
  solid3DModel->ThresholdBetween(1, 1);
  solid3DModel->Update();

  auto surface3DModel = vtkSmartPointer<vtkGeometryFilter>::New();
  surface3DModel->SetInputData(solid3DModel->GetOutput());
  surface3DModel->Update();

  auto cellIdList = vtkSmartPointer<vtkIdList>::New();
  for (vtkIdType vId = 0; vId < surface3DModel->GetOutput()->GetNumberOfCells(); vId++)
  {
    vtkIdType oriId = surface3DModel->GetOutput()->GetCellData()->GetArray("OriginalVoxelId")->GetTuple1(vId);
    cellIdList->InsertUniqueId(oriId);  // 重複なしでIDを入手
  }

  // 3Dモデル表面のみに解析属性を付与
  for (vtkIdType vId = 0; vId < cellIdList->GetNumberOfIds(); vId++)
  {
    vtkIdType surfId = cellIdList->GetId(vId);
    this->analyticProperties->SetValue(surfId, Surface3DModel);
  }

}

void vtkSettingAnalyticProperties::SetPropertiesOfWalls()
{
  // X方向の両端にある壁
  for (vtkIdType zId = 0; zId < cellDims[2]; zId++)
    for (vtkIdType yId = 0; yId < cellDims[1]; yId++)
    {
      vtkIdType minX = 0;
      vtkIdType vIdMinX = vtkIndexUtility::ConvertXYZToVtkCellId(minX, yId, zId, cellDims);

      vtkIdType maxX = cellDims[0] - 1;
      vtkIdType vIdMaxX = vtkIndexUtility::ConvertXYZToVtkCellId(maxX, yId, zId, cellDims);

      this->analyticProperties->SetValue(vIdMinX, WallX);
      this->analyticProperties->SetValue(vIdMaxX, WallX);
    }

  // Y方向の両端にある壁
  for (vtkIdType zId = 0; zId < cellDims[2]; zId++)
    for (vtkIdType xId = 0; xId< cellDims[0]; xId++)
    {
      vtkIdType minY = 0;
      vtkIdType vIdMinY = vtkIndexUtility::ConvertXYZToVtkCellId(xId, minY, zId, cellDims);

      vtkIdType maxY = cellDims[1] - 1;
      vtkIdType vIdMaxY = vtkIndexUtility::ConvertXYZToVtkCellId(xId, maxY, zId, cellDims);

      this->analyticProperties->SetValue(vIdMinY, WallY);
      this->analyticProperties->SetValue(vIdMaxY, WallY);
    }

  // Z方向の両端にある壁
  for (vtkIdType yId = 0; yId < cellDims[1]; yId++)
    for (vtkIdType xId = 0; xId < cellDims[0]; xId++)
    {
      vtkIdType minZ = 0;
      vtkIdType vIdMinZ = vtkIndexUtility::ConvertXYZToVtkCellId(xId, yId, minZ, cellDims);

      vtkIdType maxZ = cellDims[2] - 1;
      vtkIdType vIdMaxZ = vtkIndexUtility::ConvertXYZToVtkCellId(xId, yId, maxZ, cellDims);

      this->analyticProperties->SetValue(vIdMinZ, WallZ);
      this->analyticProperties->SetValue(vIdMaxZ, WallZ);
    }

}

