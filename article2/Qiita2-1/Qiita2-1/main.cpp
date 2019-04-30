#include <vtkImplicitPolyDataDistance.h>

#include <vtkGeometryFilter.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkStructuredGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkAppendFilter.h>
#include <vtkCellCenters.h>
#include <vtkIdList.h>
#include <vtkExtractCells.h>

#include <vtkDataSetWriter.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkDataSetReader.h>

#include <vtkSmartPointer.h>

#define WRITE_XML

using namespace std;

void GenerateBaseMeshFromAABB(vtkDataSet *out_baseMesh, vtkPolyData *in_polyData, const int in_cellDims[3]);
void ExtractVoxelMeshWithinClosedSurface(vtkDataSet *out_voxelMesh, vtkPoints *in_voxelCenterPoints, vtkDataSet *in_baseGrid, vtkPolyData *in_closedSurface);
const bool IsInsideOrOutside(const double *in_points, vtkPolyData *in_closedSurface, vtkImplicitPolyDataDistance *in_implicitPolyDataDistance);

int main()
{
  // 閉曲面読み取り
  auto reader = vtkSmartPointer<vtkDataSetReader>::New();
  reader->SetFileName("..\\data\\ushi.vtk");
  reader->Update();

  // PolyDataに変換
  auto geometry = vtkSmartPointer<vtkGeometryFilter>::New();
  geometry->SetInputData(reader->GetOutput());
  geometry->Update();

  auto baseMesh = vtkSmartPointer<vtkUnstructuredGrid>::New();  // 基礎メッシュ

  const int cellDims[] = { 50, 50, 50 };
  GenerateBaseMeshFromAABB(baseMesh, geometry->GetOutput(), cellDims);

  {
#ifndef WRITE_XML
    auto writer = vtkSmartPointer<vtkDataSetWriter>::New();
    writer->SetFileName("..\\data\\BaseMesh.vtk");
#else
    auto writer = vtkSmartPointer<vtkXMLDataSetWriter>::New();
    writer->SetFileName("..\\data\\BaseMesh.vtu");
#endif
    writer->SetInputData(baseMesh);
    writer->Update();
  }

  // 全ボクセル中心座標を取得
  //auto cellCenters = vtkSmartPointer<vtkCellCenters>::New();
  vtkSmartPointer<vtkCellCenters> cellCenters = vtkSmartPointer<vtkCellCenters>::New();
  cellCenters->SetInputData(baseMesh);
  cellCenters->Update();

  vtkPoints *voxelCenterPoints = cellCenters->GetOutput()->GetPoints();

  // 閉曲面に囲まれたボクセル群の形状
  auto voxelsWithinClosedSurface = vtkSmartPointer<vtkUnstructuredGrid>::New(); 

  ExtractVoxelMeshWithinClosedSurface(voxelsWithinClosedSurface, voxelCenterPoints, baseMesh, geometry->GetOutput());

  {
#ifndef WRITE_XML
    auto writer = vtkSmartPointer<vtkDataSetWriter>::New();
    writer->SetFileName("..\\data\\VoxelsWithinClosedSurface.vtk");
#else
    auto writer = vtkSmartPointer<vtkXMLDataSetWriter>::New();
    writer->SetFileName("..\\data\\VoxelsWithinClosedSurface.vtu");
#endif
    writer->SetInputData(voxelsWithinClosedSurface);
    writer->Update();
  }
}

// 閉曲面のAABB(Axis-Aligned Bounding Box)から基礎メッシュを作成
// 第1引数に空の形状データを与えること
void GenerateBaseMeshFromAABB(vtkDataSet *out_baseMesh, vtkPolyData *in_polyData, const int in_cellDims[3])
{
  // 諸元
  //double bounds[6] = { 0.0 };
  double *bounds;
  //bounds = in_polyData->GetBounds();
  {
	  //in_polyData->GetBounds(bounds);
	  bounds = in_polyData->GetBounds();
  }
  const double meshPitches[3] =
  {
    (bounds[1] - bounds[0]) / in_cellDims[0],
    (bounds[3] - bounds[2]) / in_cellDims[1],
    (bounds[5] - bounds[4]) / in_cellDims[2]
  };
  const double mins[3] = { bounds[0], bounds[2], bounds[4] };

  // 等間隔格子点生成
  auto points = vtkSmartPointer<vtkPoints>::New();
  for (int zId = 0; zId < in_cellDims[2] + 1; zId++)
  {
    for (int yId = 0; yId < in_cellDims[1] + 1; yId++)
    {
      for (int xId = 0; xId < in_cellDims[0] + 1; xId++)
      {
        const double x = xId * meshPitches[0] + mins[0];
        const double y = yId * meshPitches[1] + mins[1];
        const double z = zId * meshPitches[2] + mins[2];
        points->InsertNextPoint(x, y, z);
      }
    }
  }

  // 閉曲面を包括するAABB内に構造格子を生成
  auto baseMesh = vtkSmartPointer<vtkStructuredGrid>::New();
  baseMesh->SetExtent(0, in_cellDims[0], 0, in_cellDims[1], 0, in_cellDims[2]);
  baseMesh->SetPoints(points);

  // 何かと使いにくいことがあるので、vtkStructuredGridをvtkUnstructuredGridに変換
  auto append = vtkSmartPointer<vtkAppendFilter>::New();
  append->AddInputData(baseMesh);
  append->Update();

  // 第1引数にコピー
  out_baseMesh->DeepCopy(append->GetOutput());
}

// 基礎メッシュ内のボクセルのうち、閉曲面以内にあるものだけを抽出
// 第1引数に空の形状データを与えること
void ExtractVoxelMeshWithinClosedSurface(vtkDataSet *out_voxelMesh, vtkPoints *in_voxelCenterPoints, vtkDataSet *in_baseGrid, vtkPolyData *in_closedSurface)
{
  // 符号付き距離を測る準備
  auto implicitPolyDataDistance =
    vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
  implicitPolyDataDistance->SetInput(in_closedSurface);

  // 基礎メッシュ中の全ボクセルを参照
  auto cellIdList = vtkSmartPointer<vtkIdList>::New();
  //for (vtkIdType bgId = 0; bgId < in_baseGrid->GetNumberOfCells(); bgId++)
  for (vtkIdType bgId = 0; bgId < in_voxelCenterPoints->GetNumberOfPoints(); bgId++)
  {
    const double *currentCenter = in_voxelCenterPoints->GetPoint(bgId);
    if (IsInsideOrOutside(currentCenter, in_closedSurface, implicitPolyDataDistance))
      cellIdList->InsertNextId(bgId);
  }

  // cellIdListに登録したボクセルのみ抽出する
  auto extractCells = vtkSmartPointer<vtkExtractCells>::New();
  extractCells->SetInputData(in_baseGrid);
  extractCells->SetCellList(cellIdList);
  extractCells->Update();

  out_voxelMesh->DeepCopy(extractCells->GetOutput());
}

// 内外判定
const bool IsInsideOrOutside(const double *in_points, vtkPolyData *in_closedSurface, vtkImplicitPolyDataDistance *in_implicitPolyDataDistance)
{
  double distance = in_implicitPolyDataDistance->FunctionValue(in_points);

  if (distance <= 0)
    return true;
  else
    return false;
}
