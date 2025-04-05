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

#include "io/DefParser.h"
#include "io/DiskIO.h"
#include "io/PathMatcher.h"
#include "io/TestParserStatus.h"
#include "io/TraversalMode.h"
#include "mdl/EntityDefinition.h"
#include "mdl/EntityDefinitionTestUtils.h"
#include "mdl/EntityProperties.h"
#include "mdl/PropertyDefinition.h"

#include "Catch2.h"

namespace tb::io
{

TEST_CASE("DefParserTest.parseIncludedDefFiles")
{
  const auto basePath = std::filesystem::current_path() / "fixture/games/";
  const auto cfgFiles =
    Disk::find(basePath, TraversalMode::Flat, makeExtensionPathMatcher({".def"}))
    | kdl::value();

  for (const auto& path : cfgFiles)
  {
    CAPTURE(path);

    auto file = Disk::openFile(path) | kdl::value();
    auto reader = file->reader().buffer();
    auto parser = DefParser{reader.stringView(), Color{1.0f, 1.0f, 1.0f, 1.0f}};

    auto status = TestParserStatus{};
    CHECK(parser.parseDefinitions(status).is_success());

    /* Disabled because our files are full of previously undetected problems
    if (status.countStatus(LogLevel::Warn) > 0u) {
        UNSCOPED_INFO("Parsing DEF file " << path.string() << " produced warnings");
        for (const auto& message : status.messages(LogLevel::Warn)) {
            UNSCOPED_INFO(message);
        }
        CHECK(status.countStatus(LogLevel::Warn) == 0u);
    }

    if (status.countStatus(LogLevel::Error) > 0u) {
        UNSCOPED_INFO("Parsing DEF file " << path.string() << " produced errors");
        for (const auto& message : status.messages(LogLevel::Error)) {
            UNSCOPED_INFO(message);
        }
        CHECK(status.countStatus(LogLevel::Error) == 0u);
    }
    */
  }
}

TEST_CASE("DefParserTest.parseExtraDefFiles")
{
  const auto basePath = std::filesystem::current_path() / "fixture/test/io/Def";
  const auto cfgFiles =
    Disk::find(basePath, TraversalMode::Recursive, makeExtensionPathMatcher({".def"}))
    | kdl::value();

  for (const auto& path : cfgFiles)
  {
    auto file = Disk::openFile(path) | kdl::value();
    auto reader = file->reader().buffer();
    auto parser = DefParser{reader.stringView(), Color{1.0f, 1.0f, 1.0f, 1.0f}};

    auto status = TestParserStatus{};
    CHECK(parser.parseDefinitions(status).is_success());
    CHECK(status.countStatus(LogLevel::Warn) == 0u);
    CHECK(status.countStatus(LogLevel::Error) == 0u);
  }
}

TEST_CASE("DefParserTest.parseEmptyFile")
{
  const auto file = R"()";
  auto parser = DefParser{file, Color{1.0f, 1.0f, 1.0f, 1.0f}};

  auto status = TestParserStatus{};
  auto definitions = parser.parseDefinitions(status);
  CHECK(definitions.value().empty());
}

TEST_CASE("DefParserTest.parseWhitespaceFile")
{
  const auto file = R"(     
  	 
  )";

  auto parser = DefParser{file, Color{1.0f, 1.0f, 1.0f, 1.0f}};

  auto status = TestParserStatus{};
  auto definitions = parser.parseDefinitions(status);
  CHECK(definitions.value().empty());
}

TEST_CASE("DefParserTest.parseCommentsFile")
{
  const auto file = R"(// asdfasdfasdf
//kj3k4jkdjfkjdf
)";

  auto parser = DefParser{file, Color{1.0f, 1.0f, 1.0f, 1.0f}};

  auto status = TestParserStatus{};
  auto definitions = parser.parseDefinitions(status);
  CHECK(definitions.value().empty());
}

TEST_CASE("DefParserTest.parseSolidClass")
{
  const auto file = R"(
/*QUAKED worldspawn (0.0 0.0 0.0) ?
{
choice "worldtype"
  (
  (0,"medieval")
  (1,"metal")
  (2,"base")
  );
}
Only used for the world entity. 
Set message to the level name. 
Set sounds to the cd track to play. 
"worldtype"	type of world
*/
)";

  auto parser = DefParser{file, Color{1.0f, 1.0f, 1.0f, 1.0f}};

  auto status = TestParserStatus{};
  auto definitions = parser.parseDefinitions(status);
  CHECK(definitions.value().size() == 1u);

  const auto& definition = *definitions.value()[0];
  CHECK(definition.type() == mdl::EntityDefinitionType::BrushEntity);
  CHECK(definition.name() == "worldspawn");
  CHECK(definition.color() == Color{0.0f, 0.0f, 0.0f, 1.0f});
  CHECK(definition.description() == R"(Only used for the world entity. 
Set message to the level name. 
Set sounds to the cd track to play. 
"worldtype"	type of world)");

  const auto& properties = definition.propertyDefinitions();
  CHECK(properties.size() == 1u);
}

TEST_CASE("DefParserTest.parsePointClass")
{
  const auto file = R"(
    /*QUAKED monster_zombie (1.0 0.0 0.0) (-16 -16 -24) (16 16 32) Crucified ambush
    If crucified, stick the bounding box 12 pixels back into a wall to look right.
    */
)";

  auto parser = DefParser{file, Color{1.0f, 1.0f, 1.0f, 1.0f}};

  auto status = TestParserStatus{};
  auto definitions = parser.parseDefinitions(status);
  CHECK(definitions.value().size() == 1u);

  const auto& definition = *definitions.value()[0];
  CHECK(definition.type() == mdl::EntityDefinitionType::PointEntity);
  CHECK(definition.name() == "monster_zombie");
  CHECK(definition.color() == Color{1.0f, 0.0f, 0.0f, 1.0f});
  CHECK(
    definition.description()
    == R"(If crucified, stick the bounding box 12 pixels back into a wall to look right.)");

  const auto& pointDefinition =
    static_cast<const mdl::PointEntityDefinition&>(definition);
  CHECK(
    pointDefinition.bounds() == vm::bbox3d{{-16.0, -16.0, -24.0}, {16.0, 16.0, 32.0}});

  const auto& properties = definition.propertyDefinitions();
  CHECK(properties.size() == 1u); // spawnflags

  const auto property = properties[0];
  CHECK(property->type() == mdl::PropertyDefinitionType::FlagsProperty);

  const auto* spawnflags = definition.spawnflags();
  CHECK(spawnflags != nullptr);
  CHECK(spawnflags->defaultValue() == 0);

  CHECK(
    spawnflags->options()
    == std::vector<mdl::FlagsPropertyOption>{
      {1, "Crucified", "", false},
      {2, "ambush", "", false},
    });
}

TEST_CASE("DefParserTest.parseSpawnflagWithSkip")
{
  const auto file = R"(
    /*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16) - SUSPENDED SPIN - RESPAWN
    some desc
    */)";

  auto parser = DefParser{file, Color{1.0f, 1.0f, 1.0f, 1.0f}};

  auto status = TestParserStatus{};
  auto definitions = parser.parseDefinitions(status);
  CHECK(definitions.value().size() == 1u);

  const auto& definition = *definitions.value()[0];
  CHECK(definition.type() == mdl::EntityDefinitionType::PointEntity);
  CHECK(definition.name() == "item_health");
  CHECK(definition.color() == Color{0.3f, 0.3f, 1.0f, 1.0f});
  CHECK(definition.description() == "some desc");

  const auto& pointDefinition =
    static_cast<const mdl::PointEntityDefinition&>(definition);
  CHECK(
    pointDefinition.bounds() == vm::bbox3d{{-16.0, -16.0, -16.0}, {16.0, 16.0, 16.0}});

  const auto& properties = definition.propertyDefinitions();
  CHECK(properties.size() == 1u); // spawnflags

  const auto property = properties[0];
  CHECK(property->type() == mdl::PropertyDefinitionType::FlagsProperty);

  const auto* spawnflags = definition.spawnflags();
  CHECK(spawnflags != nullptr);
  CHECK(spawnflags->defaultValue() == 0);

  CHECK(
    spawnflags->options()
    == std::vector<mdl::FlagsPropertyOption>{
      {1, "", "", false},
      {2, "SUSPENDED", "", false},
      {4, "SPIN", "", false},
      {8, "", "", false},
      {16, "RESPAWN", "", false},
    });
}

TEST_CASE("DefParserTest.parseBrushEntityWithMissingBBoxAndNoQuestionMark")
{
  const auto file = R"(
    /*QUAKED item_health (.3 .3 1) SUSPENDED SPIN - RESPAWN
    some desc
    */)";

  auto parser = DefParser{file, Color{1.0f, 1.0f, 1.0f, 1.0f}};

  auto status = TestParserStatus{};
  auto definitions = parser.parseDefinitions(status);
  CHECK(definitions.value().size() == 1u);

  const auto& definition = *definitions.value()[0];
  CHECK(definition.type() == mdl::EntityDefinitionType::BrushEntity);
  CHECK(definition.name() == "item_health");
  CHECK(definition.color() == Color{0.3f, 0.3f, 1.0f, 1.0f});
  CHECK(definition.description() == "some desc");

  const auto& properties = definition.propertyDefinitions();
  CHECK(properties.size() == 1u); // spawnflags

  const auto property = properties[0];
  CHECK(property->type() == mdl::PropertyDefinitionType::FlagsProperty);

  const auto* spawnflags = definition.spawnflags();
  CHECK(spawnflags != nullptr);
  CHECK(spawnflags->defaultValue() == 0);

  CHECK(
    spawnflags->options()
    == std::vector<mdl::FlagsPropertyOption>{
      {1, "SUSPENDED", "", false},
      {2, "SPIN", "", false},
      {4, "", "", false},
      {8, "RESPAWN", "", false},
    });
}

TEST_CASE("DefParserTest.parsePointClassWithBaseClasses")
{
  const auto file = R"-(
    /*QUAKED _light_style
    {
    choice "style"
     (
      (0,"normal")
      (1,"flicker (first variety)")
      (2,"slow strong pulse")
      (3,"candle (first variety)")
      (4,"fast strobe")
      (5,"gentle pulse 1")
      (6,"flicker (second variety)")
      (7,"candle (second variety)")
      (8,"candle (third variety)")
      (9,"slow strobe (fourth variety)")
      (10,"fluorescent flicker")
      (11,"slow pulse not fade to black")
     );
}
    */
    
    /*QUAKED light (0.0 1.0 0.0) (-8 -8 -8) (8 8 8) START_OFF
    {
    base("_light_style");
    }
    Non-displayed light.
    Default light value is 300
    If targeted, it will toggle between on or off.
    Default "style" is 0.
    */)-";

  auto parser = DefParser{file, Color{1.0f, 1.0f, 1.0f, 1.0f}};

  auto status = TestParserStatus{};
  auto definitions = parser.parseDefinitions(status);
  CHECK(definitions.value().size() == 1u);

  const auto& definition = *definitions.value()[0];
  CHECK(definition.type() == mdl::EntityDefinitionType::PointEntity);
  CHECK(definition.name() == "light");

  CHECK(definition.propertyDefinitions().size() == 2u);

  const auto* stylePropertyDefinition = definition.propertyDefinition("style");
  CHECK(stylePropertyDefinition != nullptr);
  CHECK(stylePropertyDefinition->key() == "style");
  CHECK(stylePropertyDefinition->type() == mdl::PropertyDefinitionType::ChoiceProperty);

  const auto* spawnflagsPropertyDefinition =
    definition.propertyDefinition(mdl::EntityPropertyKeys::Spawnflags);
  CHECK(spawnflagsPropertyDefinition != nullptr);
  CHECK(spawnflagsPropertyDefinition->key() == mdl::EntityPropertyKeys::Spawnflags);
  CHECK(
    spawnflagsPropertyDefinition->type() == mdl::PropertyDefinitionType::FlagsProperty);

  const auto* choice =
    static_cast<const mdl::ChoicePropertyDefinition*>(stylePropertyDefinition);

  CHECK(
    choice->options()
    == std::vector<mdl::ChoicePropertyOption>{
      {"0", "normal"},
      {"1", "flicker (first variety)"},
      {"2", "slow strong pulse"},
      {"3", "candle (first variety)"},
      {"4", "fast strobe"},
      {"5", "gentle pulse 1"},
      {"6", "flicker (second variety)"},
      {"7", "candle (second variety)"},
      {"8", "candle (third variety)"},
      {"9", "slow strobe (fourth variety)"},
      {"10", "fluorescent flicker"},
      {"11", "slow pulse not fade to black"},
    });
}

static const auto DefModelDefinitionTemplate = R"(
  /*QUAKED monster_zombie (1.0 0.0 0.0) (-16 -16 -24) (16 16 32) Crucified ambush
  {
  model(${MODEL});
  }
  */)";

using mdl::getModelSpecification;

TEST_CASE("DefParserTest.parseLegacyStaticModelDefinition")
{
  static const auto ModelDefinition =
    R"(":maps/b_shell0.bsp", ":maps/b_shell1.bsp" spawnflags = 1)";

  CHECK(
    getModelSpecification<DefParser>(ModelDefinition, DefModelDefinitionTemplate)
    == mdl::ModelSpecification{"maps/b_shell0.bsp", 0, 0});
  CHECK(
    getModelSpecification<DefParser>(
      ModelDefinition, DefModelDefinitionTemplate, "{ 'spawnflags': 1 }")
    == mdl::ModelSpecification{"maps/b_shell1.bsp", 0, 0});
}

TEST_CASE("DefParserTest.parseLegacyDynamicModelDefinition")
{
  static const auto ModelDefinition =
    R"(pathKey = "model" skinKey = "skin" frameKey = "frame")";

  CHECK(
    getModelSpecification<DefParser>(
      ModelDefinition, DefModelDefinitionTemplate, "{ 'model': 'maps/b_shell1.bsp' }")
    == mdl::ModelSpecification{"maps/b_shell1.bsp", 0, 0});
  CHECK(
    getModelSpecification<DefParser>(
      ModelDefinition,
      DefModelDefinitionTemplate,
      "{ 'model': 'maps/b_shell1.bsp', 'skin': 1, 'frame': 2 }")
    == mdl::ModelSpecification{"maps/b_shell1.bsp", 1, 2});
}

TEST_CASE("DefParserTest.parseELModelDefinition")
{
  static const std::string ModelDefinition =
    R"({{ spawnflags == 1 -> 'maps/b_shell1.bsp', 'maps/b_shell0.bsp' }})";

  CHECK(
    getModelSpecification<DefParser>(ModelDefinition, DefModelDefinitionTemplate)
    == mdl::ModelSpecification{"maps/b_shell0.bsp", 0, 0});
}

TEST_CASE("DefParserTest.parseInvalidBounds")
{
  const std::string file = R"(
    /*QUAKED light (0.0 1.0 0.0) (8 -8 -8) (-8 8 8) START_OFF
    {
    base("_light_style");
    }
    Non-displayed light.
    Default light value is 300
    If targeted, it will toggle between on or off.
    Default "style" is 0.
    */)";

  auto parser = DefParser{file, Color{1.0f, 1.0f, 1.0f, 1.0f}};

  auto status = TestParserStatus{};
  auto definitions = parser.parseDefinitions(status);
  CHECK(definitions.value().size() == 1u);

  const auto& definition =
    static_cast<mdl::PointEntityDefinition&>(*definitions.value()[0]);
  CHECK(definition.bounds() == vm::bbox3d{8.0});
}

} // namespace tb::io
