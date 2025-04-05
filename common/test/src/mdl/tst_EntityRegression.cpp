/*
 Copyright (C) 2021 Kristian Duske

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

#include "Color.h"
#include "el/Expression.h"
#include "io/ELParser.h"
#include "mdl/Entity.h"
#include "mdl/EntityDefinition.h"

#include "Catch2.h"

namespace tb::mdl
{

TEST_CASE("EntityTest.modelScaleExpressionThrows")
{
  // see https://github.com/TrenchBroom/TrenchBroom/issues/3914

  const auto modelExpression = io::ELParser{io::ELParser::Mode::Strict, R"(
{{
    spawnflags & 2 ->   ":maps/b_bh100.bsp",
    spawnflags & 1 ->   ":maps/b_bh10.bsp",
                        ":maps/b_bh25.bsp"
}})"}
                                 .parse()
                                 .value();

  auto definition = PointEntityDefinition{
    "some_name", Color{}, vm::bbox3d{32.0}, "", {}, ModelDefinition{modelExpression}, {}};

  auto entity = Entity{};
  entity.setDefinition(&definition);

  // throws because 'a & 2' cannot be evaluated -- we must catch the exception in
  // Entity::updateCachedProperties
  CHECK_NOTHROW(entity.addOrUpdateProperty("spawnflags", "a"));
}

} // namespace tb::mdl
