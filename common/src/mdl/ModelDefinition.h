/*
 Copyright (C) 2010 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Result.h"
#include "el/Expression.h"
#include "mdl/ModelSpecification.h"

#include "kdl/reflection_decl.h"

#include "vm/vec.h"

#include <optional>

namespace tb
{
struct FileLocation;
}

namespace tb::mdl
{

namespace ModelSpecificationKeys
{
constexpr auto Path = "path";
constexpr auto Skin = "skin";
constexpr auto Frame = "frame";
constexpr auto Scale = "scale";
} // namespace ModelSpecificationKeys

class ModelDefinition
{
private:
  el::ExpressionNode m_expression;

public:
  ModelDefinition();
  explicit ModelDefinition(const FileLocation& location);

  explicit ModelDefinition(el::ExpressionNode expression);

  void append(ModelDefinition other);

  /**
   * Evaluates the model expresion, using the given variable store to interpolate
   * variables.
   *
   * @param variableStore the variable store to use when interpolating variables
   * @return the model specification or an error if evaluation failed
   */
  Result<ModelSpecification> modelSpecification(
    const el::VariableStore& variableStore) const;

  /**
   * Evaluates the model expresion.
   *
   * @return the model specification or an error if evaluation failed
   */
  Result<ModelSpecification> defaultModelSpecification() const;

  /**
   * Evaluates the model expression using the given variable store to interpolate
   * variables, and returns the scale value configured for the model, if any. If the model
   * expression doesn't have its own scale expression, then the given scale expression is
   * used instead.
   *
   * @return the scale or an error if evaluation failed
   */
  Result<vm::vec3d> scale(
    const el::VariableStore& variableStore,
    const std::optional<el::ExpressionNode>& defaultScaleExpression) const;

  kdl_reflect_decl(ModelDefinition, m_expression);
};

/**
 * Returns the model scale value for the given parameters or a default scale of 1, 1, 1 if
 * an error occurs.
 */
vm::vec3d safeGetModelScale(
  const ModelDefinition& definition,
  const el::VariableStore& variableStore,
  const std::optional<el::ExpressionNode>& defaultScaleExpression);

} // namespace tb::mdl
