#pragma once

#include "GView.hpp"

namespace GView
{
namespace Type
{
    namespace YAML
    {
        namespace TokenType
        {
            constexpr uint32 dash                   = 0;
            constexpr uint32 comment                = 1;
            constexpr uint32 scalarBlock            = 2;
            constexpr uint32 documentSeparatorStart = 3;
            constexpr uint32 documentSeparatorEnd   = 4;
            constexpr uint32 type                   = 5;
            constexpr uint32 tag                    = 6;
            constexpr uint32 scalarValue            = 7;
            constexpr uint32 comma                  = 8;
            constexpr uint32 inlineList             = 9;
            constexpr uint32 associative_array      = 10;
            constexpr uint32 indentation            = 11;
            constexpr uint32 newLine                = 12;
            constexpr uint32 invalid                = 13;
        }; // namespace TokenType

        class YAMLFile : public TypeInterface, public GView::View::LexicalViewer::ParseInterface
        {
            void ParseFile(GView::View::LexicalViewer::SyntaxManager& syntax);
            void BuildBlocks(GView::View::LexicalViewer::SyntaxManager& syntax);

          public:
            YAMLFile();
            virtual ~YAMLFile()
            {
            }

            std::string_view GetTypeName() override
            {
                return "YAML";
            }
            void RunCommand(std::string_view) override
            {
            }
            virtual bool UpdateKeys(KeyboardControlsInterface* interface) override
            {
                return true;
            }
            virtual void GetTokenIDStringRepresentation(uint32 id, AppCUI::Utils::String& str) override;
            virtual void PreprocessText(GView::View::LexicalViewer::TextEditor& editor) override;
            virtual void AnalyzeText(GView::View::LexicalViewer::SyntaxManager& syntax) override;
            virtual bool StringToContent(std::u16string_view string, AppCUI::Utils::UnicodeStringBuilder& result) override;
            virtual bool ContentToString(std::u16string_view content, AppCUI::Utils::UnicodeStringBuilder& result) override;

          public:
            Reference<GView::Utils::SelectionZoneInterface> selectionZoneInterface;

            uint32 GetSelectionZonesCount() override
            {
                CHECK(selectionZoneInterface.IsValid(), 0, "");
                return selectionZoneInterface->GetSelectionZonesCount();
            }

            TypeInterface::SelectionZone GetSelectionZone(uint32 index) override
            {
                static auto d = TypeInterface::SelectionZone{ 0, 0 };
                CHECK(selectionZoneInterface.IsValid(), d, "");
                CHECK(index < selectionZoneInterface->GetSelectionZonesCount(), d, "");

                return selectionZoneInterface->GetSelectionZone(index);
            }

            GView::Utils::JsonBuilderInterface* GetSmartAssistantContext(const std::string_view& prompt, std::string_view displayPrompt) override;
        };

        namespace Panels
        {
            class Information : public AppCUI::Controls::TabPage
            {
                Reference<GView::Type::YAML::YAMLFile> yaml;
                Reference<AppCUI::Controls::ListView> general;
                Reference<AppCUI::Controls::ListView> issues;

                void UpdateGeneralInformation();
                void UpdateIssues();
                void RecomputePanelsPositions();

              public:
                Information(Reference<GView::Type::YAML::YAMLFile> yaml);

                void Update();
                virtual void OnAfterResize(int newWidth, int newHeight) override
                {
                    RecomputePanelsPositions();
                }
            };
        }; // namespace Panels
    }      // namespace YAML
} // namespace Type
} // namespace GView
