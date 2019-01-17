#include "FDMSolverForHeatEq.h"
#include "SettingAnalyticProperties.h"
#include "VtkIndexUtility.h"

#include <vtkXMLUnstructuredGridReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDataSetWriter.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkCellData.h>
#include <vtkCell.h>

#include <vtkSmartPointer.h>

using namespace std;

FDMSolverForHeatEq::FDMSolverForHeatEq()
{
}

FDMSolverForHeatEq::~FDMSolverForHeatEq()
{
}

// �݌v�K���ł���������
// �������F��͏�̏���ۑ��C�������F��͏ꒆ�̏�Q���\�ʂ̏ꏊ�C��O�����F���̒l�ŏI���X�e�b�v�����
void FDMSolverForHeatEq::SolveHeatEquationForAllStep(vtkUnstructuredGrid *out_field, const int &in_endStep)
{
  vtkIdType NumCellsInfield = out_field->GetNumberOfCells();
  //const double alpha = 0.01;
  SetParameters(out_field, alpha); // ��̓p�����[�^
  auto initResults = vtkSmartPointer<vtkDoubleArray>::New();
  initResults->SetName("Results");
  initResults->SetNumberOfComponents(1);
  for (auto vId = 0; vId < NumCellsInfield; vId++)
    initResults->InsertNextTuple1(0.0);

  out_field->GetCellData()->AddArray(initResults);

  // �v�Z
  int step;
  for (step = 0; step < in_endStep; step++)
  {
    if (!SolveHeatEquationForOneStep(out_field))
      break;
  }
  cout << step << "�ŏI��" << endl;
}

// �Ȃ񂩂��������Ƃ��Ă邩������Ȃ����ǋ����Ă���
bool FDMSolverForHeatEq::SolveHeatEquationForOneStep(vtkUnstructuredGrid *out_field)
{
  // ��O�̌��ʂ����o��
  auto beforeResults = vtkSmartPointer<vtkDoubleArray>::New();
  beforeResults->DeepCopy(vtkDoubleArray::SafeDownCast(out_field->GetCellData()->GetArray("Results")));
  if (!beforeResults) return false;

  // ��͑���
  auto properties = out_field->GetCellData()->GetArray("AnalyticProoerty");
  if (!properties)
  {
    cout << "��͗p�̑������{�N�Z���ɐݒ肳��Ă��܂���B" << endl;
    return false;
  }

  // ���E����
  SetBoundryCondition(beforeResults, properties);

  // ���̌v�Z����
  auto nextResults = vtkSmartPointer<vtkDoubleArray>::New();
  nextResults->DeepCopy(beforeResults);

  // �S�{�N�Z�����Q��
  for (vtkIdType vId = 0; vId < out_field->GetNumberOfCells(); vId++)
  {
    const vtkIdType *xyzId = VtkIndexUtility::ConvertVtkCellIdToXYZ(vId, cellDims);

    // ��͑�����Calculation�ȊO�Ȃ��΂��B
    if (properties->GetTuple1(vId) != Analysis)
      continue;

    const double beforeRes = beforeResults->GetTuple1(vId);

    // TODO�F�ϐ����ύX
    const vtkIdType *peripheralXYZs = VtkIndexUtility::GetPeripheralXYZs(xyzId[0], xyzId[1], xyzId[2], cellDims);

    const double deltas[3] = { this->deltas[0], this->deltas[1], this->deltas[2] };

    // �}�Ƀ����_�g���Ă��܂Ȃ�
    auto getRes = [&](const vtkIdType &cellId) -> const double
    { return beforeResults->GetTuple1(cellId); };

    // �v�Z��
    const double nextRes = beforeRes + this->dt * this->alpha *
      (
      (getRes(peripheralXYZs[0]) - 2.0 * beforeRes + getRes(peripheralXYZs[1])) / (deltas[0] * deltas[0]) +
        (getRes(peripheralXYZs[2]) - 2.0 * beforeRes + getRes(peripheralXYZs[3])) / (deltas[1] * deltas[1]) +
        (getRes(peripheralXYZs[4]) - 2.0 * beforeRes + getRes(peripheralXYZs[5])) / (deltas[2] * deltas[2])
        );

    nextResults->SetTuple1(vId, nextRes);
  }

  out_field->GetCellData()->AddArray(nextResults);
  return true;
}

void FDMSolverForHeatEq::SetParameters(vtkUnstructuredGrid *out_field, const double &in_alpha)
{
  // ��̓p�����[�^ ----
  double *fieldBounds = out_field->GetBounds();
  // out_field �̊e�����̑傫��
  this->lengths[0] = fieldBounds[1] - fieldBounds[0];
  this->lengths[1] = fieldBounds[3] - fieldBounds[2];
  this->lengths[2] = fieldBounds[5] - fieldBounds[4];
  // (���Ԋu�i�q�̏ꍇ)�ǂ�ł������̂ŃZ�����Ƃ��Ă��Ă��̑傫���𒲂ׂ�
  vtkCell *voxel = out_field->GetCell(0);
  const double voxelSizes[3] =
  {
    voxel->GetBounds()[1] - voxel->GetBounds()[0],
    voxel->GetBounds()[3] - voxel->GetBounds()[2],
    voxel->GetBounds()[5] - voxel->GetBounds()[4]
  };
  // XYZ�e�����̃Z�����𒲂ׂ�
  this->cellDims[0] = round(this->lengths[0] / voxelSizes[0]);
  this->cellDims[1] = round(this->lengths[1] / voxelSizes[1]);
  this->cellDims[2] = round(this->lengths[2] / voxelSizes[2]);

  // ��ԍ��ݕ�
  this->deltas[0] = this->lengths[0] / this->cellDims[0];
  this->deltas[1] = this->lengths[1] / this->cellDims[1];
  this->deltas[2] = this->lengths[2] / this->cellDims[2];

  this->alpha = in_alpha;   // > 0

  // �t�H���E�m�C�}���̈��萫��͂Ɋ�Â����ԍ��ݕ���ݒ�
  this->dt = 0.5 / (in_alpha *
    (
      1.0 / (this->deltas[0] * this->deltas[0]) +
      1.0 / (this->deltas[1] * this->deltas[1]) +
      1.0 / (this->deltas[2] * this->deltas[2])
      ));
}

// ���ӂ̃{�N�Z�����畽�ϒl�����߂�
void FDMSolverForHeatEq::SetAveValueFromSurroundings(const vtkIdType in_voxelId, vtkDoubleArray *out_results, vtkDataArray *in_properties)
{
  const vtkIdType *xyzId = VtkIndexUtility::ConvertVtkCellIdToXYZ(in_voxelId, cellDims);

  // �{�N�Z������6�̕��ς��Ƃ�

  const vtkIdType *peripheralXYZs = VtkIndexUtility::GetPeripheralXYZs(xyzId[0], xyzId[1], xyzId[2], cellDims);

  int count = 0;
  double sumResult = 0.0;
  for (vtkIdType pXyzId = 0; pXyzId < 6; pXyzId++)
  {
    vtkIdType vId = peripheralXYZs[pXyzId];

    // �z�񒆂̊m�ۂ��Ă���v�f�ȊO�͎Q�Ƃ��Ȃ�
    if (vId < 0 || out_results->GetNumberOfTuples() <= vId)
      continue;
    if (in_properties->GetTuple1(vId) != Analysis)
      continue;

    sumResult += out_results->GetValue(vId);
    count++;
  }

  out_results->SetValue(in_voxelId, sumResult / count);
}

void FDMSolverForHeatEq::SetBoundryCondition(vtkDoubleArray * in_results, vtkDataArray * in_properties)
{
  // �S�{�N�Z���Q��
  for (auto vId = 0; vId < in_properties->GetNumberOfTuples(); vId++)
  {
    vtkIdType propId = in_properties->GetTuple1(vId);

    // 3D���f���\�ʂȂ�A���͂̑�����Calculation�̃{�N�Z���ɒ�`����Ă���v�Z�l�̕��ϒl���Z�b�g
    if (propId == Surface3DModel)
      SetAveValueFromSurroundings(vId, in_results, in_properties);
    // ��
    else if (propId == WallX)
      in_results->SetTuple1(vId, wallXYZBC[0]);
    else if (propId == WallY)
      in_results->SetTuple1(vId, wallXYZBC[1]);
    else if (propId == WallZ)
      in_results->SetTuple1(vId, wallXYZBC[2]);
  }

}

