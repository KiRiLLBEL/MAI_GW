import {TextDocument} from 'vscode-languageserver-textdocument';
import {
  createConnection,
  Diagnostic,
  DiagnosticSeverity,
  InitializeParams,
  InitializeResult,
  ProposedFeatures,
  SemanticTokens,
  SemanticTokensBuilder,
  SemanticTokensParams,
  TextDocuments,
  TextDocumentSyncKind
} from 'vscode-languageserver/node';

const connection = createConnection(ProposedFeatures.all);

const documents: TextDocuments<TextDocument> = new TextDocuments(TextDocument);

import {ExecException, spawn} from 'child_process';
import * as path from 'path';

function parseMyLanguage(inputText: string): Promise<any> {
  return new Promise((resolve, reject) => {
    const binaryPath =
        path.join(__dirname, '../path_to_cpp_binary/mydsl-parser');

    const args = [ '-f', '-', '-o', '-', '-t', 'json' ];

    const child = spawn(binaryPath, args, {stdio : [ 'pipe', 'pipe', 'pipe' ]});

    let stdoutData = '';
    let stderrData = '';

    child.stdout.on('data', (data) => { stdoutData += data.toString(); });
    child.stderr.on('data', (data) => { stderrData += data.toString(); });

    child.on('close', (code) => {
      if (code !== 0) {
        return reject(new Error(`Process return code ${code}: ${stderrData}`));
      }
      try {
        const ast = JSON.parse(stdoutData);
        resolve(ast);
      } catch (e) {
        reject(e);
      }
    });

    child.stdin.write(inputText);
    child.stdin.end();
  });
}

const tokenTypes = [ 'variable', 'function', "superset" ];
const tokenModifiers: string[] = [];
function validateAndAnalyzeDocument(doc: TextDocument): void {
  const text = doc.getText();
  let ast: any;
  try {
    ast = parseMyLanguage(text);
  } catch (e: any) {
    const diag: Diagnostic = {
      severity : DiagnosticSeverity.Error,
      range :
          {start : {line : 0, character : 0}, end : {line : 0, character : 1}},
      message : `AST exec error: ${e.message}`,
      source : "parser"
    };
    connection.sendDiagnostics({uri : doc.uri, diagnostics : [ diag ]});
    return;
  }
  const diagnostics: Diagnostic[] = [];
  performSemanticAnalysis(ast, diagnostics);

  connection.sendDiagnostics({uri : doc.uri, diagnostics});
}

connection.onInitialize((params: InitializeParams): InitializeResult => {
  return {
    capabilities : {
      textDocumentSync : TextDocumentSyncKind.Full,
      semanticTokensProvider :
          {legend : {tokenTypes, tokenModifiers}, full : true}
    }
  };
});

connection.onInitialized(() => { console.log('LSP-server initialized.'); });

documents.listen(connection);

documents.onDidOpen(event => { validateAndAnalyzeDocument(event.document); });

documents.onDidChangeContent(
    change => { validateAndAnalyzeDocument(change.document); });

function populateSemanticTokens(node: any,
                                builder: SemanticTokensBuilder): void {
  switch (node.type) {
  case 'Keyword':
    addToken(node, 'keyword');
    break;
  case 'Identifier':
    if (!isDeclared(node.name)) {
      addToken(node, 'variable');
    }
    break;
  case 'CallExpression':
    addToken(node.identifierNode, 'function');
    break;
  }
  for (const child of node.children ?? []) {
    populateSemanticTokens(child, builder);
  }

  function addToken(node: any, tokenType: string) {
    builder.push(node.position.line - 1, node.position.colStart - 1,
                 node.position.colEnd - node.position.colStart,
                 tokenTypes.indexOf(tokenType), 0);
  }
}

connection.onRequest("textDocument/semanticTokens/full",
                     (params: SemanticTokensParams): SemanticTokens => {
                       const doc = documents.get(params.textDocument.uri);
                       if (!doc) {
                         return {data : []};
                       }
                       const text = doc.getText();
                       let ast: any;
                       try {
                         ast = parseMyLanguage(text);
                       } catch {
                         return {data : []};
                       }
                       const builder = new SemanticTokensBuilder();
                       populateSemanticTokens(ast, builder);
                       return builder.build();
                     });

connection.listen();
