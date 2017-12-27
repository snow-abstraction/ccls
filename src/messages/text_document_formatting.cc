#include "clang_format.h"
#include "message_handler.h"

namespace {
struct lsFormattingOptions {
  // Size of a tab in spaces.
  int tabSize;
  // Prefer spaces over tabs.
  bool insertSpaces;
};
MAKE_REFLECT_STRUCT(lsFormattingOptions, tabSize, insertSpaces);

struct lsTextDocumentFormattingParams {
  // The text document.
  lsTextDocumentIdentifier textDocument;

  // The format options, like tabs or spaces.
  lsFormattingOptions formattingOptions;
};
MAKE_REFLECT_STRUCT(lsTextDocumentFormattingParams,
                    textDocument,
                    formattingOptions);

struct Ipc_TextDocumentFormatting
    : public IpcMessage<Ipc_TextDocumentFormatting> {
  const static IpcId kIpcId = IpcId::TextDocumentFormatting;

  lsRequestId id;
  lsTextDocumentFormattingParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentFormatting, id, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentFormatting);

struct Out_TextDocumentFormatting
    : public lsOutMessage<Out_TextDocumentFormatting> {
  lsRequestId id;
  std::vector<lsTextEdit> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentFormatting, jsonrpc, id, result);

struct TextDocumentFormattingHandler
    : BaseMessageHandler<Ipc_TextDocumentFormatting> {
  void Run(Ipc_TextDocumentFormatting* request) override {
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    int tab_size = request->params.formattingOptions.tabSize;
    bool insert_spaces = request->params.formattingOptions.insertSpaces;

    Out_TextDocumentFormatting response;
    response.id = request->id;

    const auto clang_format = std::unique_ptr<ClangFormat>(new ClangFormat(
        working_file->filename, working_file->buffer_content,
        {clang::tooling::Range(0, working_file->buffer_content.size())},
        tab_size, insert_spaces));
    const auto replacements = clang_format->FormatWholeDocument();
    response.result = ConvertClangReplacementsIntoTextEdits(
        working_file->buffer_content, replacements);

    QueueManager::WriteStdout(IpcId::TextDocumentFormatting, response);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentFormattingHandler);
}  // namespace
