#include "yaml.hpp"

using namespace AppCUI;
using namespace AppCUI::Utils;
using namespace AppCUI::Application;
using namespace AppCUI::Controls;
using namespace GView::Utils;
using namespace GView::Type;
using namespace GView;
using namespace GView::View;

extern "C" {
PLUGIN_EXPORT bool Validate(const BufferView&, const std::string_view& extension)
{
    return extension == ".yaml" || extension == ".yml";
}

PLUGIN_EXPORT TypeInterface* CreateInstance()
{
    return new YAML::YAMLFile();
}
PLUGIN_EXPORT bool PopulateWindow(Reference<WindowInterface> win)
{
    auto yaml = win->GetObject()->GetContentType<YAML::YAMLFile>();

    LexicalViewer::Settings settings;
    settings.SetParser(yaml.ToObjectRef<LexicalViewer::ParseInterface>());

    win->CreateViewer(settings);

    win->CreateViewer<TextViewer::Settings>("Text View");

    GView::View::BufferViewer::Settings s{};
    yaml->selectionZoneInterface = win->GetSelectionZoneInterfaceFromViewerCreation(s);

    //// add panels
    win->AddPanel(Pointer<TabPage>(new YAML::Panels::Information(yaml)), true);

    return true;
}
PLUGIN_EXPORT void UpdateSettings(IniSection sect)
{
    sect["Extension"]   = "yaml";
    sect["Priority"]    = 1;
    sect["Description"] = "YAML Ain't Markup Language File Format (*.yaml)";
}

}

