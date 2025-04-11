import {spawn} from 'child_process';
import * as path from 'path';
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

import {performSemanticAnalysis} from './analysis';

const connection = createConnection(ProposedFeatures.all);
const documents: TextDocuments<TextDocument> = new TextDocuments(TextDocument);

function parseMyLanguage(inputText: string): Promise<any> {
  return new Promise((resolve, reject) => {
    const binaryPath = path.join(__dirname, '../dsl-parser');
    const args = [ '-f', '-', '-o', '-', '-t', 'json' ];

    connection.console.log(
        `Parser start "${binaryPath}" with arguments: ${args.join(' ')}`);

    const child = spawn(binaryPath, args, {stdio : [ 'pipe', 'pipe', 'pipe' ]});

    let stdoutData = '';
    let stderrData = '';

    child.stdout.on('data', (data) => { stdoutData += data.toString(); });
    child.stderr.on('data', (data) => { stderrData += data.toString(); });
    child.stdin.on('error', (err) => {
      connection.console.error(`Error on child.stdin: ${err.message}`);
    });
    child.on('close', (code) => {
      connection.console.log(`Parser ended with code: ${code}`);
      if (code !== 0) {
        connection.console.error(`Parser error: ${stderrData}`);
        return reject(new Error(`Process return code ${code}: ${stderrData}`));
      }
      try {
        const ast = JSON.parse(stdoutData);
        resolve(ast);
      } catch (e) {
        connection.console.error(
            `Failed to parse JSON: ${(e as Error).message}`);
        reject(e);
      }
    });

    child.stdin.write(inputText, (err) => {
      if (err) {
        connection.console.error(`Error writing to stdin: ${err.message}`);
      }
      child.stdin.end();
    });
  });
}

async function validateAndAnalyzeDocument(doc: TextDocument): Promise<void> {
  const text = doc.getText();
  let ast: any;
  try {
    ast = await parseMyLanguage(text);
  } catch (e: any) {
    connection.console.error(`AST exec error: ${e.message}`);
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
  try {
    await performSemanticAnalysis(ast, diagnostics, [ new Set<string>() ]);
  } catch (err: any) {
    connection.console.error(`Semantic analysis error: ${err.message}`);
  }
  connection.sendDiagnostics({uri : doc.uri, diagnostics});
}

connection.onInitialize((params: InitializeParams): InitializeResult => {
  connection.console.log('Initializing LSP-server...');
  return {
    capabilities : {
      textDocumentSync : TextDocumentSyncKind.Full,
      semanticTokensProvider : {
        legend : {
          tokenTypes : [ 'variable', 'function', 'superset' ],
          tokenModifiers : []
        },
        full : true
      }
    }
  };
});

connection.onInitialized(
    () => { connection.console.log('LSP-server initialized.'); });

documents.listen(connection);

documents.onDidOpen(event => {
  connection.console.log(`Open file: ${event.document.uri}`);
  validateAndAnalyzeDocument(event.document)
      .catch(err => connection.console.error(`Error processing document ${
                 event.document.uri}: ${err.message}`));
});

documents.onDidChangeContent(change => {
  validateAndAnalyzeDocument(change.document)
      .catch(err => connection.console.error(`Error processing change in ${
                 change.document.uri}: ${err.message}`));
});

async function populateSemanticTokens(
    node: any, builder: SemanticTokensBuilder): Promise<void> {
  const addToken = (node: any, tokenType: string) => {
    if (!node.node)
      return;
    const line = node.node.line;
    const character = node.node.column;
    const length = node.node.length;
    const tokenTypeIndex =
        [ 'variable', 'function', 'superset' ].indexOf(tokenType);
    if (tokenTypeIndex === -1)
      return;
    builder.push(line, character, length, tokenTypeIndex, 0);
    connection.console.log(`Add token: type=${tokenType} (index=${
        tokenTypeIndex}), line=${line}, char=${character}, length=${length}`);
  };

  switch (node.type) {
  case 'keyword':
    addToken(node, 'superset');
    break;
  case 'variable':
    addToken(node, 'variable');
    break;
  case 'call':
    addToken(node, 'function');
    break;
  default:
    break;
  }

  if (node.hasOwnProperty('children') && Array.isArray(node.children)) {
    for (const child of node.children) {
      await populateSemanticTokens(child, builder);
    }
  }

  if (node.hasOwnProperty('blocks')) {
    await populateSemanticTokens(node.blocks, builder);
  }

  if (node.hasOwnProperty('statements') && Array.isArray(node.statements)) {
    for (const child of node.statements) {
      await populateSemanticTokens(child, builder);
    }
  }

  if (node.hasOwnProperty('args') && Array.isArray(node.args)) {
    for (const child of node.args) {
      await populateSemanticTokens(child, builder);
    }
  }

  if (node.hasOwnProperty('statement')) {
    await populateSemanticTokens(node.statement, builder);
  }

  if (node.hasOwnProperty('expression')) {
    await populateSemanticTokens(node.expression, builder);
  }

  if (node.hasOwnProperty('quantifier')) {
    await populateSemanticTokens(node.quantifier, builder);
  }

  if (node.hasOwnProperty('cond')) {
    await populateSemanticTokens(node.cond, builder);
  }
  if (node.hasOwnProperty('then')) {
    await populateSemanticTokens(node.then, builder);
  }
  if (node.hasOwnProperty('else')) {
    await populateSemanticTokens(node.else, builder);
  }
  if (node.hasOwnProperty('source')) {
    await populateSemanticTokens(node.source, builder);
  }
  if (node.hasOwnProperty('predicate')) {
    await populateSemanticTokens(node.predicate, builder);
  }

  if (node.hasOwnProperty('left')) {
    await populateSemanticTokens(node.left, builder);
  }
  if (node.hasOwnProperty('right')) {
    await populateSemanticTokens(node.right, builder);
  }

  if (node.hasOwnProperty('operand')) {
    await populateSemanticTokens(node.operand, builder);
  }
}

connection.onRequest(
    "textDocument/semanticTokens/full",
    async(params: SemanticTokensParams): Promise<SemanticTokens> => {
      const doc = documents.get(params.textDocument.uri);
      if (!doc) {
        connection.console.error(
            `Document not found: ${params.textDocument.uri}`);
        return {data : []};
      }
      const text = doc.getText();
      let ast: any;
      try {
        ast = await parseMyLanguage(text);
        connection.console.log(`AST: ${JSON.stringify(ast).substr(0, 200)}...`);
      } catch (e: any) {
        connection.console.error(`Semantic tokens parse error: ${e.message}`);
        return {data : []};
      }
      const builder = new SemanticTokensBuilder();
      await populateSemanticTokens(ast, builder);
      const tokensResult = builder.build();
      connection.console.log(
          `Semantic tokens count: ${tokensResult.data.length}`);
      return tokensResult;
    });

connection.listen();
