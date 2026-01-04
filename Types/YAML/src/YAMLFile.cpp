#include "yaml.hpp"

namespace GView::Type::YAML
{
using namespace GView::View::LexicalViewer;

namespace CharacterType
{
    constexpr uint32 open_brace    = 0;  // {
    constexpr uint32 close_brace   = 1;  // }
    constexpr uint32 open_bracket  = 2;  // [
    constexpr uint32 close_bracket = 3;  // ]
    constexpr uint32 dash          = 4;  // -
    constexpr uint32 colon         = 5;  // :
    constexpr uint32 comma         = 6;  // ,
    constexpr uint32 quote         = 7;  // ' or "
    constexpr uint32 hash          = 8;  // #
    constexpr uint32 pipe          = 9;  // |
    constexpr uint32 greater       = 10; // >
    constexpr uint32 spaces        = 11;
    constexpr uint32 newline       = 12;
    constexpr uint32 alphanum      = 13;
    constexpr uint32 invalid       = 14;

    uint32 GetCharacterType(char16 ch)
    {
        if (ch == '{')
            return open_brace;
        if (ch == '}')
            return close_brace;
        if (ch == '[')
            return open_bracket;
        if (ch == ']')
            return close_bracket;
        if (ch == '-')
            return dash;
        if (ch == ':')
            return colon;
        if (ch == ',')
            return comma;
        if (ch == '\'' || ch == '"')
            return quote;
        if (ch == '#')
            return hash;
        if (ch == '|')
            return pipe;
        if (ch == '>')
            return greater;
        if (ch == ' ' || ch == '\t')
            return spaces;
        if (ch == '\n' || ch == '\r')
            return newline;

        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '.' || ch == '/')
            return alphanum;

        return invalid;
    }
} // namespace CharacterType

#define CHAR_CASE(char_type, align)                                                                                                        \
    case CharacterType::char_type:                                                                                                         \
        syntax.tokens.Add(                                                                                                                 \
              TokenType::char_type,                                                                                                        \
              pos,                                                                                                                         \
              pos + 1,                                                                                                                     \
              TokenColor::Operator,                                                                                                        \
              TokenDataType::None,                                                                                                         \
              (align),                                                                                                                     \
              TokenFlags::DisableSimilaritySearch);                                                                                        \
        pos++;                                                                                                                             \
        break;

YAMLFile::YAMLFile()
{
}

void YAMLFile::ParseFile(SyntaxManager& syntax)
{
    auto len  = syntax.text.Len();
    auto pos  = 0u;
    auto next = 0u;

    while (pos < len) {
        auto ct = CharacterType::GetCharacterType(syntax.text[pos]);

        switch (ct) {

            CHAR_CASE(open_brace, TokenAlignament::StartsOnNewLine | TokenAlignament::NewLineAfter);
            CHAR_CASE(close_brace, TokenAlignament::StartsOnNewLine | TokenAlignament::NewLineAfter);
            CHAR_CASE(open_bracket, TokenAlignament::None);
            CHAR_CASE(close_bracket, TokenAlignament::NewLineAfter);
            CHAR_CASE(comma, TokenAlignament::NewLineAfter | TokenAlignament::AfterPreviousToken);

        case CharacterType::colon:
            syntax.tokens.Add(
                  TokenType::colon,
                  pos,
                  pos + 1,
                  TokenColor::Operator,
                  TokenAlignament::AddSpaceBefore | TokenAlignament::AddSpaceAfter | TokenAlignament::SameColumn);
            pos++;
            break;

        case CharacterType::dash:
            syntax.tokens.Add(TokenType::dash, pos, pos + 1, TokenColor::Operator, TokenAlignament::StartsOnNewLine);
            pos++;
            break;

        case CharacterType::pipe:
            syntax.tokens.Add(TokenType::block_literal, pos, pos + 1, TokenColor::Operator, TokenAlignament::AddSpaceAfter);
            pos++;
            break;

        case CharacterType::greater:
            syntax.tokens.Add(TokenType::block_folded, pos, pos + 1, TokenColor::Operator, TokenAlignament::AddSpaceAfter);
            pos++;
            break;

        case CharacterType::hash:
            next = syntax.text.ParseUntilEndOfLine(pos);
            syntax.tokens.Add(TokenType::comment, pos, next, TokenColor::Comment, TokenAlignament::StartsOnNewLine);
            pos = next;
            break;

        case CharacterType::spaces:
            next = syntax.text.ParseSpace(pos, SpaceType::SpaceAndTabs);
            syntax.tokens.Add(TokenType::whitespace, pos, next, TokenColor::Operator, TokenAlignament::None);
            pos = next;
            break;

        case CharacterType::newline:
            syntax.tokens.Add(TokenType::newline, pos, pos + 1, TokenColor::Operator, TokenAlignament::None);
            pos++;
            break;

        case CharacterType::quote:
            next = syntax.text.ParseString(pos, StringFormat::All);
            if (syntax.tokens.GetLastTokenID() == TokenType::colon) {
                syntax.tokens.Add(TokenType::value, pos, next, TokenColor::Word, TokenAlignament::AddSpaceBefore);
            } else {
                syntax.tokens.Add(TokenType::key, pos, next, TokenColor::Keyword, TokenAlignament::StartsOnNewLine);
            }
            pos = next;
            break;

        case CharacterType::alphanum:
            next = syntax.text.ParseSameGroupID(pos, CharacterType::GetCharacterType);
            if (syntax.tokens.GetLastTokenID() == TokenType::colon) {
                syntax.tokens.Add(TokenType::value, pos, next, TokenColor::Word, TokenAlignament::AddSpaceBefore);
            } else {
                syntax.tokens.Add(TokenType::key, pos, next, TokenColor::Keyword, TokenAlignament::StartsOnNewLine);
            }
            pos = next;
            break;

        default:
            next = syntax.text.ParseSameGroupID(pos, CharacterType::GetCharacterType);
            syntax.tokens.Add(TokenType::invalid, pos, next, TokenColor::Error, TokenAlignament::AddSpaceBefore).SetError("Invalid character for YAML file");
            pos = next;
            break;
        }
    }
}

void YAMLFile::BuildBlocks(GView::View::LexicalViewer::SyntaxManager& syntax)
{
    TokenIndexStack mapStack;
    TokenIndexStack seqStack;
    TokenIndexStack indentStack;

    const auto len = syntax.tokens.Len();

    uint32 lastIndent         = 0;
    uint32 lastLineStartToken = 0;

    for (uint32 i = 0; i < len; i++) {
        auto type = syntax.tokens[i].GetTypeID(TokenType::invalid);

        switch (type) {
        case TokenType::open_brace:
            mapStack.Push(i);
            break;

        case TokenType::close_brace:
            if (!mapStack.Empty()) {
                auto start = mapStack.Pop();
                syntax.blocks.Add(start, i, BlockAlignament::ParentBlockWithIndent, BlockFlags::EndMarker);
            } else {
                syntax.tokens[i].SetError("Unexpected '}'");
            }
            break;

        case TokenType::open_bracket:
            seqStack.Push(i);
            break;

        case TokenType::close_bracket:
            if (!seqStack.Empty()) {
                auto start = seqStack.Pop();
                syntax.blocks.Add(start, i, BlockAlignament::CurrentToken, BlockFlags::EndMarker);
            } else {
                syntax.tokens[i].SetError("Unexpected ']'");
            }
            break;
        }

        if (type == TokenType::newline) {
            lastLineStartToken = i + 1;
            continue;
        }

        if (type == TokenType::whitespace) {
            uint32 indent = syntax.tokens[i].GetTokenEndOffset().value() - syntax.tokens[i].GetTokenStartOffset().value();

            if (indent > lastIndent) {
                indentStack.Push(lastLineStartToken);
            } else if (indent < lastIndent) {
                while (!indentStack.Empty() && indent < lastIndent) {
                    auto start = indentStack.Pop();
                    syntax.blocks.Add(start, i - 1, BlockAlignament::ParentBlockWithIndent, BlockFlags::EndMarker);
                    lastIndent = indent;
                }
            }

            lastIndent = indent;
        }
    }

    while (!indentStack.Empty()) {
        auto start = indentStack.Pop();
        syntax.blocks.Add(start, len - 1, BlockAlignament::ParentBlockWithIndent, BlockFlags::EndMarker);
    }
}

void YAMLFile::PreprocessText(GView::View::LexicalViewer::TextEditor&)
{
    // nothing to do --> there is no pre-processing needed for a YAML format
}
void YAMLFile::GetTokenIDStringRepresentation(uint32 id, AppCUI::Utils::String& str)
{
    switch (id) {
    case TokenType::key:
        str = "Key";
        break;
    case TokenType::value:
        str = "Value";
        break;
    case TokenType::colon:
        str = "Colon";
        break;
    case TokenType::dash:
        str = "Sequence Item (-)";
        break;
    case TokenType::indentation:
        str = "Indentation";
        break;
    case TokenType::newline:
        str = "New Line";
        break;
    case TokenType::string:
        str = "String";
        break;
    case TokenType::number:
        str = "Number";
        break;
    case TokenType::boolean:
        str = "Boolean";
        break;
    case TokenType::null_value:
        str = "Null";
        break;
    case TokenType::block_literal:
        str = "Block Literal (|)";
        break;
    case TokenType::block_folded:
        str = "Block Folded (>)";
        break;
    case TokenType::open_brace:
        str = "Mapping Start ({)";
        break;
    case TokenType::close_brace:
        str = "Mapping End (})";
        break;
    case TokenType::open_bracket:
        str = "Sequence Start ([)";
        break;
    case TokenType::close_bracket:
        str = "Sequence End (])";
        break;
    case TokenType::comma:
        str = "Comma";
        break;
    case TokenType::comment:
        str = "Comment";
        break;
    case TokenType::anchor:
        str = "Anchor (&)";
        break;
    case TokenType::alias:
        str = "Alias (*)";
        break;
    case TokenType::tag:
        str = "Tag (!!)";
        break;
    case TokenType::document_start:
        str = "Document Start (---)";
        break;
    case TokenType::document_end:
        str = "Document End (...)";
        break;
    case TokenType::whitespace:
        str = "Whitespace";
        break;
    case TokenType::invalid:
        str = "Invalid";
        break;
    default:
        str.SetFormat("Unknown Token (0x%08X)", id);
        break;
    }
}
void YAMLFile::AnalyzeText(GView::View::LexicalViewer::SyntaxManager& syntax)
{
    ParseFile(syntax);
    BuildBlocks(syntax);
}
bool YAMLFile::StringToContent(std::u16string_view string, AppCUI::Utils::UnicodeStringBuilder& result)
{
    return TextParser::ExtractContentFromString(string, result, StringFormat::All);
}
bool YAMLFile::ContentToString(std::u16string_view content, AppCUI::Utils::UnicodeStringBuilder& result)
{
    NOT_IMPLEMENTED(false);
}

GView::Utils::JsonBuilderInterface* YAMLFile::GetSmartAssistantContext(const std::string_view& prompt, std::string_view displayPrompt)
{
    auto builder = GView::Utils::JsonBuilderInterface::Create();
    builder->AddU16String("Name", obj->GetName());
    builder->AddUInt("ContentSize", obj->GetData().GetSize());
    return builder;
}

} // namespace GView::Type::YAML