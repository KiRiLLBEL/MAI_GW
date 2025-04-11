import { spawn } from 'child_process';
import * as path from 'path';
import { TextDocument } from 'vscode-languageserver-textdocument';
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

import { performSemanticAnalysis } from './analysis';

const DEBUG = false;

const connection = createConnection(ProposedFeatures.all);
const documents: TextDocuments<TextDocument> = new TextDocuments(TextDocument);

// Кэш AST для документов: uri -> { version, ast }
const astCache = new Map<string, { version: number, ast: any }>();

function parseMyLanguage(inputText: string): Promise<any> {
  return new Promise((resolve, reject) => {
    const binaryPath = path.join(__dirname, '../dsl-parser');
    const args = ['-f', '-', '-o', '-', '-t', 'json'];

    if (DEBUG) {
      connection.console.log(`Запуск парсера "${binaryPath}" с аргументами: ${args.join(' ')}`);
    }

    const child = spawn(binaryPath, args, { stdio: ['pipe', 'pipe', 'pipe'] });
    let stdoutData = '';
    let stderrData = '';

    child.stdout.on('data', (data) => { stdoutData += data.toString(); });
    child.stderr.on('data', (data) => { stderrData += data.toString(); });
    child.stdin.on('error', (err) => {
      connection.console.error(`Ошибка при записи в stdin: ${err.message}`);
    });
    child.on('close', (code) => {
      if (DEBUG) {
        connection.console.log(`Парсер завершился с кодом: ${code}`);
      }
      if (code !== 0) {
        connection.console.error(`Ошибка парсера: ${stderrData}`);
        return reject(new Error(`Код возврата ${code}: ${stderrData}`));
      }
      try {
        const ast = JSON.parse(stdoutData);
        resolve(ast);
      } catch (e) {
        connection.console.error(`Ошибка парсинга JSON: ${(e as Error).message}`);
        reject(e);
      }
    });

    child.stdin.write(inputText, (err) => {
      if (err) {
        connection.console.error(`Ошибка записи в stdin: ${err.message}`);
      }
      child.stdin.end();
    });
  });
}

async function getAST(doc: TextDocument): Promise<any> {
  const cached = astCache.get(doc.uri);
  if (cached && cached.version === doc.version) {
    if (DEBUG) {
      connection.console.log(`Используется кэшированный AST для ${doc.uri}`);
    }
    return cached.ast;
  }
  const ast = await parseMyLanguage(doc.getText());
  astCache.set(doc.uri, { version: doc.version, ast });
  return ast;
}

async function validateAndAnalyzeDocument(doc: TextDocument): Promise<void> {
  let ast: any;
  try {
    ast = await getAST(doc);
  } catch (e: any) {
    connection.console.error(`Ошибка выполнения AST: ${e.message}`);
    const diag: Diagnostic = {
      severity: DiagnosticSeverity.Error,
      range: { start: { line: 0, character: 0 }, end: { line: 0, character: 1 } },
      message: `Ошибка AST: ${e.message}`,
      source: "parser"
    };
    connection.sendDiagnostics({ uri: doc.uri, diagnostics: [diag] });
    return;
  }
  const diagnostics: Diagnostic[] = [];
  try {
    await performSemanticAnalysis(ast, diagnostics, [new Set<string>()]);
  } catch (err: any) {
    connection.console.error(`Ошибка семантического анализа: ${err.message}`);
  }
  connection.sendDiagnostics({ uri: doc.uri, diagnostics });
}

connection.onInitialize((params: InitializeParams): InitializeResult => {
  connection.console.log('Инициализация LSP-сервера...');
  return {
    capabilities: {
      textDocumentSync: TextDocumentSyncKind.Full,
      semanticTokensProvider: {
        legend: {
          tokenTypes: ['variable', 'function', 'superset'],
          tokenModifiers: []
        },
        full: true
      }
    }
  };
});

connection.onInitialized(() => {
  connection.console.log('LSP-сервер инициализирован.');
});

documents.listen(connection);

const debounceTimers = new Map<string, NodeJS.Timeout>();
const DEBOUNCE_DELAY = 300; // мс

documents.onDidOpen(event => {
  connection.console.log(`Открыт файл: ${event.document.uri}`);
  validateAndAnalyzeDocument(event.document)
    .catch(err => connection.console.error(`Ошибка обработки документа ${event.document.uri}: ${err.message}`));
});

documents.onDidChangeContent(change => {
  const uri = change.document.uri;
  if (debounceTimers.has(uri)) {
    clearTimeout(debounceTimers.get(uri));
  }
  const timer = setTimeout(() => {
    validateAndAnalyzeDocument(change.document)
      .catch(err => connection.console.error(`Ошибка обработки изменений в ${uri}: ${err.message}`));
    debounceTimers.delete(uri);
  }, DEBOUNCE_DELAY);
  debounceTimers.set(uri, timer);
});

connection.onRequest("textDocument/semanticTokens/full", async (params: SemanticTokensParams): Promise<SemanticTokens> => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc) {
    connection.console.error(`Документ не найден: ${params.textDocument.uri}`);
    return { data: [] };
  }
  let ast: any;
  try {
    ast = await getAST(doc);
    if (DEBUG) {
      connection.console.log(`AST: ${JSON.stringify(ast).substr(0, 200)}...`);
    }
  } catch (e: any) {
    connection.console.error(`Ошибка получения семантических токенов: ${e.message}`);
    return { data: [] };
  }
  const builder = new SemanticTokensBuilder();
  const tokensResult = builder.build();
  connection.console.log(`Количество семантических токенов: ${tokensResult.data.length}`);
  return tokensResult;
});

connection.listen();