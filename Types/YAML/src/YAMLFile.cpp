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
    constexpr uint32 singleQuote   = 7;  // '
    constexpr uint32 doubleQuote   = 8;  // "
    constexpr uint32 hash          = 9;  // #
    constexpr uint32 pipe          = 10; // |
    constexpr uint32 greater       = 11; // >
    constexpr uint32 tag           = 12; // &
    constexpr uint32 reference     = 13; // *
    constexpr uint32 type          = 14; // !
    constexpr uint32 dot           = 15; // .
    constexpr uint32 spaces        = 16;
    constexpr uint32 newline       = 17;
    constexpr uint32 alphanum      = 18;
    constexpr uint32 invalid       = 19;

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
        if (ch == '\'')
            return singleQuote;
        if (ch == '"')
            return doubleQuote;
        if (ch == '#')
            return hash;
        if (ch == '|')
            return pipe;
        if (ch == '>')
            return greater;
        if (ch == '&')
            return tag;
        if (ch == '*')
            return reference;
        if (ch == '!')
            return type;
        if (ch == '.')
            return dot;
        if (ch == ' ' || ch == '\t')
            return spaces;
        if (ch == '\n' || ch == '\r')
            return newline;

        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '.' || ch == '/')
            return alphanum;

        return invalid;
    }
} // namespace CharacterType

#define CHAR_CASE(char_type, align)                                                                                                                            \
    case CharacterType::char_type:                                                                                                                             \
        syntax.tokens.Add(TokenType::char_type, pos, pos + 1, TokenColor::Operator, TokenDataType::None, (align), TokenFlags::DisableSimilaritySearch);        \
        pos++;                                                                                                                                                 \
        break;

YAMLFile::YAMLFile()
{
}
void YAMLFile::ParseFile(SyntaxManager& syntax)
{
    auto len         = syntax.text.Len();
    uint32 pos       = 0;
    bool atLineStart = true;

    std::vector<std::string> tokenOrder;

    while (pos < len) {
        auto ct = CharacterType::GetCharacterType(syntax.text[pos]);

        switch (ct) {
        case (CharacterType::dash): {
            auto next = syntax.text.ParseSameGroupID(pos, CharacterType::GetCharacterType);
            if (atLineStart && next - pos == 3) {
                syntax.tokens.Add(
                      TokenType::documentSeparatorStart, pos, next, TokenColor::Comment, TokenAlignament::StartsOnNewLine | TokenAlignament::NewLineAfter);
            } else {
                syntax.tokens.Add(TokenType::dash, pos, pos + 1, TokenColor::Operator, TokenAlignament::AddSpaceAfter);
            }
            pos         = next;
            atLineStart = false;
            break;
        }

        case (CharacterType::dot): {
            auto next = syntax.text.ParseSameGroupID(pos, CharacterType::GetCharacterType);
            if (atLineStart && next - pos == 3) {
                syntax.tokens.Add(
                      TokenType::documentSeparatorEnd, pos, next, TokenColor::Comment, TokenAlignament::StartsOnNewLine | TokenAlignament::NewLineAfter);
            } else {
                syntax.tokens.Add(TokenType::scalarValue, pos, pos + 1, TokenColor::Word, TokenAlignament::AddSpaceAfter);
            }
            pos         = next;
            atLineStart = false;
            break;
        }

        case (CharacterType::hash): {
            auto next = syntax.text.ParseUntilEndOfLine(pos);
            syntax.tokens.Add(TokenType::comment, pos, next, TokenColor::Comment, TokenAlignament::StartsOnNewLine | TokenAlignament::NewLineAfter);
            pos         = next;
            atLineStart = false;
            break;
        }

        case (CharacterType::singleQuote): {
            auto next = syntax.text.ParseString(pos, StringFormat::SingleQuotes);
            syntax.tokens.Add(TokenType::scalarValue, pos, next, TokenColor::String, TokenAlignament::None);
            pos         = next;
            atLineStart = false;
            break;
        }

        case (CharacterType::doubleQuote): {
            auto next = syntax.text.ParseString(pos, StringFormat::DoubleQuotes);
            syntax.tokens.Add(TokenType::scalarValue, pos, next, TokenColor::String, TokenAlignament::None);
            pos         = next;
            atLineStart = false;
            break;
        }

        case (CharacterType::colon): {
            syntax.tokens.Add(TokenType::colon, pos, pos + 1, TokenColor::Word, TokenAlignament::AddSpaceAfter);
            pos++;
            atLineStart = false;
            break;
        }

        case (CharacterType::pipe):
        case (CharacterType::greater): {
            syntax.tokens.Add(TokenType::scalarBlock, pos, pos + 1, TokenColor::Operator, TokenAlignament::AddSpaceAfter);
            pos++;
            atLineStart = false;
            break;
        }

        case (CharacterType::newline): {
            auto next = syntax.text.ParseSpace(pos, SpaceType::NewLine);
            syntax.tokens.Add(TokenType::newLine, pos, next, TokenColor::Operator, TokenAlignament::NewLineAfter);
            pos         = next;
            atLineStart = true;
            break;
        }

        case (CharacterType::tag):
        case (CharacterType::reference): {
            auto next = syntax.text.ParseUntilEndOfLine(pos);
            syntax.tokens.Add(TokenType::tag, pos, next, TokenColor::Constant, TokenAlignament::None);
            pos         = next;
            atLineStart = false;
            break;
        }

        case (CharacterType::open_brace):
        case (CharacterType::close_brace): {
            syntax.tokens.Add(TokenType::associative_array, pos, pos + 1, TokenColor::Operator, TokenAlignament::None);
            pos++;
            atLineStart = false;
            break;
        }

        case (CharacterType::open_bracket):
        case (CharacterType::close_bracket): {
            syntax.tokens.Add(TokenType::inlineList, pos, pos + 1, TokenColor::Operator, TokenAlignament::None);
            pos++;
            atLineStart = false;
            break;
        }

        case (CharacterType::comma): {
            syntax.tokens.Add(TokenType::comma, pos, pos + 1, TokenColor::Operator, TokenAlignament::AddSpaceAfter);
            pos++;
            atLineStart = false;
            break;
        }

        case (CharacterType::spaces): {
            auto next = syntax.text.ParseSpace(pos, SpaceType::SpaceAndTabs);
            if (atLineStart) {
                syntax.tokens.Add(TokenType::indentation, pos, next, TokenColor::Operator, TokenAlignament::None);
            }
            pos = next;
            break;
        }

        case (CharacterType::alphanum): {
            auto next = syntax.text.ParseSameGroupID(pos, CharacterType::GetCharacterType);
            syntax.tokens.Add(TokenType::scalarValue, pos, next, TokenColor::Word, TokenAlignament::AddSpaceAfter);
            pos         = next;
            atLineStart = false;
            break;
        }

        default: {
            auto next = syntax.text.ParseSameGroupID(pos, CharacterType::GetCharacterType);
            syntax.tokens.Add(TokenType::invalid, pos, next, TokenColor::Error, TokenAlignament::None);
            pos         = next;
            atLineStart = false;
            break;
        }
        }
    }

}


void YAMLFile::BuildBlocks(GView::View::LexicalViewer::SyntaxManager& syntax)
{
    struct Block {
        uint32 start;
        uint32 indent;
        uint32 line;
    };

    std::vector<Block> stack;

    auto len = syntax.tokens.Len();

    uint32 currentIndent = 0;
    uint32 currentLine   = 0;
    uint32 consecutiveLines = 0;
    bool sameLine = false;

    for (uint32 index = 0; index < len; index++) {
        auto typeID = syntax.tokens[index].GetTypeID(TokenType::invalid);

        if (typeID == TokenType::newLine) {
            currentLine++;
            consecutiveLines++;
            currentIndent = 0;
            sameLine      = false;
            continue;
        }

        if (typeID == TokenType::scalarBlock) {
            continue;
        }

        if (sameLine && typeID == TokenType::scalarValue) {
            continue;
        }

        if (typeID == TokenType::indentation) {
            currentIndent = syntax.tokens[index].GetTokenEndOffset().value() - syntax.tokens[index].GetTokenStartOffset().value();
            continue;
        }

        while (!stack.empty() && currentIndent <= stack.back().indent) {
            auto blk = stack.back();
            stack.pop_back();

            if (currentLine > blk.line + 1) {
                auto testa = syntax.tokens[index - 1].GetTypeID(TokenType::invalid);
                auto testb = syntax.tokens[index - 2].GetTypeID(TokenType::invalid);
                syntax.blocks.Add(blk.start, index - consecutiveLines - (currentIndent > 0 ? 1 : 0), BlockAlignament::ParentBlockWithIndent, BlockFlags::EndMarker);
            }
        }

        if (typeID == TokenType::colon) {
            stack.push_back({ index, currentIndent, currentLine });
            sameLine = true;
        }

        consecutiveLines = 0;
    }

    while (!stack.empty()) {
        auto blk = stack.back();
        stack.pop_back();

        if (currentLine > blk.line + 1) {
            syntax.blocks.Add(blk.start, len - 1, BlockAlignament::ParentBlockWithIndent, BlockFlags::EndMarker);
        }
    }
}


void YAMLFile::PreprocessText(GView::View::LexicalViewer::TextEditor&)
{
    // nothing to do --> there is no pre-processing needed for a YAML format
}
void YAMLFile::GetTokenIDStringRepresentation(uint32 id, AppCUI::Utils::String& str)
{
    switch (id) {
    case TokenType::dash:
        str = "List Item (-)";
        break;

    case TokenType::comment:
        str = "Comment (#)";
        break;

    case TokenType::scalarBlock:
        str = "Block Scalar (| or >)";
        break;

    case TokenType::documentSeparatorStart:
        str = "Document Separator Start(---)";
        break;

    case TokenType::documentSeparatorEnd:
        str = "Document Separator End(...)";
        break;

    case TokenType::type:
        str = "Type Declaration";
        break;

    case TokenType::tag:
        str = "Tag (!)";
        break;

    case TokenType::scalarValue:
        str = "Scalar Value";
        break;

    case TokenType::comma:
        str = "Comma";
        break;

    case TokenType::colon:
        str = "Colon";
        break;

    case TokenType::inlineList:
        str = "Inline List ([ ])";
        break;

    case TokenType::associative_array:
        str = "Inline Mapping ({ })";
        break;

    case TokenType::indentation:
        str = "Indentation";
        break;

    case TokenType::invalid:
        str = "Invalid Token";
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