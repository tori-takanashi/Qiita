#include "VoxelMeshGeneratorByVtk.h"

//#include "pch.h"
#include <vtkImplicitPolyDataDistance.h>

#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkStructuredGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkAppendFilter.h>
#include <vtkCellCenters.h>
#include <vtkCellData.h>

VoxelMeshGeneratorByVtk::VoxelMeshGeneratorByVtk()
{
}

VoxelMeshGeneratorByVtk::~VoxelMeshGeneratorByVtk()
{
}

void VoxelMeshGeneratorByVtk::SetInputData(vtkPolyData *in_polyData)
{
  this->polyData = vtkSmartPointer<vtkPolyData>::New();
  this->polyData->DeepCopy(in_polyData);
}

void VoxelMeshGeneratorByVtk::Update()
{
  SetParameters();  // 諸元の設定
  GenerateVoxelMesh();
}

void VoxelMeshGeneratorByVtk::SetParameters()
{
  this->bounds = this->polyData->GetBounds();

#if 0
  this->meshPitches[0] = ((this->bounds[1] + this->offsets[1]) - (this->bounds[0] - this->offsets[0])) / this->cellDims[0];
  this->meshPitches[1] = ((this->bounds[3] + this->offsets[3]) - (this->bounds[2] - this->offsets[2])) / this->cellDims[1];
  this->meshPitches[2] = ((this->bounds[5] + this->offsets[5]) - (this->bounds[4] - this->offsets[4])) / this->cellDims[2];

  this->mins[0] = this->bounds[0] - this->offsets[0];
  this->mins[1] = this->bounds[2] - this->offsets[2];
  this->mins[2] = this->bounds[4] - this->offsets[4];
#else
  for (int xyz = 0; xyz < 3; xyz++)
  {
    const int plusId = 2 * xyz + 1;
    const int minusId = 2 * xyz;

    this->meshPitches[xyz] = ((this->bounds[plusId] + this->offsets[plusId]) - (this->bounds[minusId] - this->offsets[minusId])) / this->cellDims[xyz];
    this->mins[xyz] = this->bounds[minusId] - this->offsets[minusId];
  }
#endif
}

// 閉曲面のAABB(Axis-Aligned Bounding Box)から基礎メッシュを作成
// 第1引数に空の形状データを与えること
// 第4引数のオフセットには(0:x方向-側，1:x方向+側，2:y方向-側，3:y方向+側，4:z方向-側，5:z方向+側)のオフセットを与えること
void VoxelMeshGeneratorByVtk::GenerateVoxelMesh()
{
  // 等間隔格子点生成
  auto points = vtkSmartPointer<vtkPoints>::New();

  for (int zId = 0; zId < cellDims[2] + 1; zId++)
    for (int yId = 0; yId < cellDims[1] + 1; yId++)
      for (int xId = 0; xId < cellDims[0] + 1; xId++)
      {
        const double x = xId * meshPitches[0] + mins[0];
        const double y = yId * meshPitches[1] + mins[1];
        const double z = zId * meshPitches[2] + mins[2];
        points->InsertNextPoint(x, y, z);
      }

  // 閉曲面を包括するAABB内に構造格子を生成
  auto strGridBaseMesh = vtkSmartPointer<vtkStructuredGrid>::New();
  strGridBaseMesh->SetExtent(0, cellDims[0], 0, cellDims[1], 0, cellDims[2]);
  strGridBaseMesh->SetPoints(points);

  // 何かと使いにくいことがあるので、vtkStructuredGridをvtkUnstructuredGridに変換
  auto append = vtkSmartPointer<vtkAppendFilter>::New();
  append->AddInputData(strGridBaseMesh);
  append->Update();

  this->baseMesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
  this->baseMesh->DeepCopy(append->GetOutput());

  // 全ボクセル中心座標を取得
  auto cellCenters = vtkSmartPointer<vtkCellCenters>::New();
  cellCenters->SetInputData(this->baseMesh);
  cellCenters->Update();
  vtkPoints *voxelCenterPoints = cellCenters->GetOutput()->GetPoints();

  // 解析用の場に3Dモデルの位置やボクセルの通し番号を登録する。
  this->voxelMeshForAnalysis = vtkSmartPointer<vtkUnstructuredGrid>::New();
  this->voxelMeshForAnalysis->DeepCopy(append->GetOutput());
  AddIs3DModelIds(voxelCenterPoints);
  AddOriginalVoxelId();

}

// 内外判定
const bool VoxelMeshGeneratorByVtk::IsInsideOrOutside(const double *in_points, vtkImplicitPolyDataDistance *in_implicitPolyDataDistance)
{
  double distance = in_implicitPolyDataDistance->FunctionValue(in_points);

  if (distance <= 0)
    return true;
  else
    return false;
}

void VoxelMeshGeneratorByVtk::AddIs3DModelIds(vtkPoints *in_voxelCenters)
{
  auto is3DModelIds = vtkSmartPointer<vtkIdTypeArray>::New();  // 3Dモデルかどうかボクセル単位で属性をつける。
  is3DModelIds->SetName("Is3DModel");
  is3DModelIds->SetNumberOfComponents(1);

  for (int vId = 0; vId < this->voxelMeshForAnalysis->GetNumberOfCells(); vId++)
  {
    is3DModelIds->InsertNextValue(0);  // 3Dモデルがない部分を0とする。後で3Dモデルがあるところだけ1で塗りつぶす。
  }

  // ボクセルの中心点が取れてるなら以下を実行
  if (!in_voxelCenters->GetNumberOfPoints())
    return;

  // 符号付き距離を測る準備
  auto implicitPolyDataDistance =
    vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
  implicitPolyDataDistance->SetInput(polyData);

  for (vtkIdType bgId = 0; bgId < in_voxelCenters->GetNumberOfPoints(); bgId++)
  {
    const double *currentCenter = in_voxelCenters->GetPoint(bgId);

    if (IsInsideOrOutside(currentCenter, implicitPolyDataDistance))
      is3DModelIds->SetValue(bgId, 1);   // 閉曲面内部のみ1を与える。
  }

  this->voxelMeshForAnalysis->GetCellData()->AddArray(is3DModelIds);

}

void VoxelMeshGeneratorByVtk::AddOriginalVoxelId()
{
  auto originalIds = vtkSmartPointer<vtkIdTypeArray>::New();  // あとで利用するのでボクセルの通し番号を控えておく。
  originalIds->SetName("OriginalVoxelId");
  originalIds->SetNumberOfComponents(1);

  for (int vId = 0; vId < this->voxelMeshForAnalysis->GetNumberOfCells(); vId++)
  {
    originalIds->InsertNextValue(vId);
  }

  this->voxelMeshForAnalysis->GetCellData()->AddArray(originalIds);
}


vtkUnstructuredGrid *VoxelMeshGeneratorByVtk::GetOutput()
{
  return this->voxelMeshForAnalysis;
}