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
            constexpr uint32 key            = 0;
            constexpr uint32 value          = 1;
            constexpr uint32 colon          = 2;
            constexpr uint32 dash           = 3;
            constexpr uint32 indentation    = 4;
            constexpr uint32 newline        = 5;
            constexpr uint32 string         = 6;
            constexpr uint32 number         = 7;
            constexpr uint32 boolean        = 8;
            constexpr uint32 null_value     = 9;
            constexpr uint32 block_literal  = 10;
            constexpr uint32 block_folded   = 11;
            constexpr uint32 open_brace     = 12;
            constexpr uint32 close_brace    = 13;
            constexpr uint32 open_bracket   = 14;
            constexpr uint32 close_bracket  = 15;
            constexpr uint32 comma          = 16;
            constexpr uint32 comment        = 17;
            constexpr uint32 anchor         = 18;
            constexpr uint32 alias          = 19;
            constexpr uint32 tag            = 20;
            constexpr uint32 document_start = 21;
            constexpr uint32 document_end   = 22;
            constexpr uint32 whitespace     = 23; 
            constexpr uint32 invalid        = 24;
        };  // namespace TokenType

        namespace Plugins
        {
            class UpperCase : public GView::View::LexicalViewer::Plugin
            {
              public:
                virtual std::string_view GetName() override;
                virtual std::string_view GetDescription() override;
                virtual bool CanBeAppliedOn(const GView::View::LexicalViewer::PluginData& data) override;
                virtual GView::View::LexicalViewer::PluginAfterActionRequest Execute(
                      GView::View::LexicalViewer::PluginData& data, Reference<Window> parent) override;
            };
        } // namespace Plugins      

        class YAMLFile : public TypeInterface, public GView::View::LexicalViewer::ParseInterface
        {
            void ParseFile(GView::View::LexicalViewer::SyntaxManager& syntax);
            void BuildBlocks(GView::View::LexicalViewer::SyntaxManager& syntax);

          public:
            Plugins::UpperCase upper_case_plugin;

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
    } // namespace YAML
} // namespace Type
} // namespace GView
