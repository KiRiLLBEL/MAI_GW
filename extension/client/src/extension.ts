import {spawn} from 'child_process';
import neo4j from 'neo4j-driver';
import * as path from 'path';
import * as vscode from 'vscode';
import {
  LanguageClient,
  LanguageClientOptions,
  ServerOptions,
  TransportKind
} from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: vscode.ExtensionContext) {
  const serverModule = context.asAbsolutePath(path.join('out', 'server.js'));
  const serverOptions: ServerOptions = {
    run : {module : serverModule, transport : TransportKind.ipc},
    debug : {
      module : serverModule,
      transport : TransportKind.ipc,
      options : {execArgv : [ "--nolazy", "--inspect=6009" ]}
    }
  };

  const clientOptions: LanguageClientOptions = {
    documentSelector : [ {scheme : 'file', language : 'arch'} ],
    synchronize :
        {fileEvents : vscode.workspace.createFileSystemWatcher('**/*.arch')}
  };

  client = new LanguageClient('ArchLanguageServer', 'Arch Language Server',
                              serverOptions, clientOptions);

  client.start();

  context.subscriptions.push(
      vscode.commands.registerCommand('arch.run', async () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
          vscode.window.showErrorMessage('Нет открытого документа.');
          return;
        }
        const document = editor.document;
        const dslCode = document.getText();

        const config = vscode.workspace.getConfiguration('ArchLanguageServer');
        let dslParserPath: string = config.get('dslParserPath', '');
        if (!dslParserPath) {
          dslParserPath =
              context.asAbsolutePath(path.join('server', 'dsl-parser'));
        }
        const neo4jUrl: string =
            config.get('neo4j.url', 'bolt://localhost:7687');
        const neo4jUsername: string = config.get('neo4j.username', 'neo4j');
        const neo4jPassword: string = config.get('neo4j.password', '');

        const cypherQuery = await translateDSLToCypher(dslParserPath, dslCode);
        if (!cypherQuery) {
          vscode.window.showErrorMessage('Ошибка трансляции DSL в Cypher.');
          return;
        }

        try {
          const driver = neo4j.driver(
              neo4jUrl, neo4j.auth.basic(neo4jUsername, neo4jPassword));
          const session = driver.session();
          const result = await session.run(cypherQuery);
          await session.close();
          await driver.close();

          const panel = vscode.window.createWebviewPanel(
              'archGraph', 'Neo4j Graph', vscode.ViewColumn.Beside, {});
          panel.webview.html = generateGraphHtml(result.records);

        } catch (err: any) {
          vscode.window.showErrorMessage(
              `Ошибка подключения к Neo4j: ${err.message}`);
        }
      }));
}

async function translateDSLToCypher(dslParserPath: string,
                                    code: string): Promise<string|undefined> {
  return new Promise<string|undefined>((resolve) => {
    const args = [ '-f', '-', '-o', '-', '-t', 'cypher' ];
    const child =
        spawn(dslParserPath, args, {stdio : [ 'pipe', 'pipe', 'pipe' ]});
    let stdoutData = '';
    let stderrData = '';
    child.stdout.on('data', (data) => { stdoutData += data.toString(); });
    child.stderr.on('data', (data) => { stderrData += data.toString(); });
    child.on('close', (code) => {
      if (code !== 0) {
        vscode.window.showErrorMessage(`dsl-parser error: ${stderrData}`);
        return resolve(undefined);
      }
      resolve(stdoutData);
    });
    child.stdin.write(code, (err) => {
      if (err) {
        vscode.window.showErrorMessage(
            `Ошибка записи в dsl-parser stdin: ${err.message}`);
      }
      child.stdin.end();
    });
  });
}

function generateGraphHtml(records: any[]): string {
  const data = JSON.stringify(records, null, 2);
  return `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Neo4j Graph</title>
  <style>
    pre { white-space: pre-wrap; word-wrap: break-word; }
  </style>
</head>
<body>
  <h1>Результаты запроса Neo4j</h1>
  <pre>${data}</pre>
  <!-- Здесь можно подключить скрипты для визуализации графа -->
</body>
</html>`;
}

export function deactivate(): Thenable<void>|undefined {
  return client ? client.stop() : undefined;
}
