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
  SetParameters();  // �����̐ݒ�
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

// �Ȗʂ�AABB(Axis-Aligned Bounding Box)�����b���b�V�����쐬
// ��1�����ɋ�̌`��f�[�^��^���邱��
// ��4�����̃I�t�Z�b�g�ɂ�(0:x����-���C1:x����+���C2:y����-���C3:y����+���C4:z����-���C5:z����+��)�̃I�t�Z�b�g��^���邱��
void VoxelMeshGeneratorByVtk::GenerateVoxelMesh()
{
  // ���Ԋu�i�q�_����
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

  // �Ȗʂ�����AABB���ɍ\���i�q�𐶐�
  auto strGridBaseMesh = vtkSmartPointer<vtkStructuredGrid>::New();
  strGridBaseMesh->SetExtent(0, cellDims[0], 0, cellDims[1], 0, cellDims[2]);
  strGridBaseMesh->SetPoints(points);

  // �����Ǝg���ɂ������Ƃ�����̂ŁAvtkStructuredGrid��vtkUnstructuredGrid�ɕϊ�
  auto append = vtkSmartPointer<vtkAppendFilter>::New();
  append->AddInputData(strGridBaseMesh);
  append->Update();

  this->baseMesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
  this->baseMesh->DeepCopy(append->GetOutput());

  // �S�{�N�Z�����S���W���擾
  auto cellCenters = vtkSmartPointer<vtkCellCenters>::New();
  cellCenters->SetInputData(this->baseMesh);
  cellCenters->Update();
  vtkPoints *voxelCenterPoints = cellCenters->GetOutput()->GetPoints();

  // ��͗p�̏��3D���f���̈ʒu��{�N�Z���̒ʂ��ԍ���o�^����B
  this->voxelMeshForAnalysis = vtkSmartPointer<vtkUnstructuredGrid>::New();
  this->voxelMeshForAnalysis->DeepCopy(append->GetOutput());
  AddIs3DModelIds(voxelCenterPoints);
  AddOriginalVoxelId();

}

// ���O����
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
  auto is3DModelIds = vtkSmartPointer<vtkIdTypeArray>::New();  // 3D���f�����ǂ����{�N�Z���P�ʂő���������B
  is3DModelIds->SetName("Is3DModel");
  is3DModelIds->SetNumberOfComponents(1);

  for (int vId = 0; vId < this->voxelMeshForAnalysis->GetNumberOfCells(); vId++)
  {
    is3DModelIds->InsertNextValue(0);  // 3D���f�����Ȃ�������0�Ƃ���B���3D���f��������Ƃ��낾��1�œh��Ԃ��B
  }

  // �{�N�Z���̒��S�_�����Ă�Ȃ�ȉ������s
  if (!in_voxelCenters->GetNumberOfPoints())
    return;

  // �����t�������𑪂鏀��
  auto implicitPolyDataDistance =
    vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
  implicitPolyDataDistance->SetInput(polyData);

  for (vtkIdType bgId = 0; bgId < in_voxelCenters->GetNumberOfPoints(); bgId++)
  {
    const double *currentCenter = in_voxelCenters->GetPoint(bgId);

    if (IsInsideOrOutside(currentCenter, implicitPolyDataDistance))
      is3DModelIds->SetValue(bgId, 1);   // �Ȗʓ����̂�1��^����B
  }

  this->voxelMeshForAnalysis->GetCellData()->AddArray(is3DModelIds);

}

void VoxelMeshGeneratorByVtk::AddOriginalVoxelId()
{
  auto originalIds = vtkSmartPointer<vtkIdTypeArray>::New();  // ���Ƃŗ��p����̂Ń{�N�Z���̒ʂ��ԍ����T���Ă����B
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