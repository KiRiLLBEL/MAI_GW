import { spawn } from 'child_process';
import neo4j from 'neo4j-driver';
import { Record } from 'neo4j-driver';
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
    run: { module: serverModule, transport: TransportKind.ipc },
    debug: {
      module: serverModule,
      transport: TransportKind.ipc,
      options: { execArgv: ["--nolazy", "--inspect=6009"] }
    }
  };

  const clientOptions: LanguageClientOptions = {
    documentSelector: [{ scheme: 'file', language: 'arch' }],
    synchronize: { fileEvents: vscode.workspace.createFileSystemWatcher('**/*.arch') }
  };

  client = new LanguageClient('ArchLanguageServer', 'Arch Language Server', serverOptions, clientOptions);
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
        dslParserPath = context.asAbsolutePath(path.join('server', 'dsl-parser'));
      }
      const neo4jUrl: string = config.get('neo4j.url', 'bolt://localhost:7687');
      const neo4jUsername: string = config.get('neo4j.username', 'neo4j');
      const neo4jPassword: string = config.get('neo4j.password', '');

      const cypherQuery = await translateDSLToCypher(dslParserPath, dslCode);
      if (!cypherQuery) {
        vscode.window.showErrorMessage('Ошибка трансляции DSL в Cypher.');
        return;
      }

      try {
        const driver = neo4j.driver(neo4jUrl, neo4j.auth.basic(neo4jUsername, neo4jPassword));
        const session = driver.session();
        const result = await session.run(cypherQuery);
        await session.close();
        await driver.close();

        const panel = vscode.window.createWebviewPanel(
          'archGraph',
          'Neo4j Graph',
          vscode.ViewColumn.Beside,
          { enableScripts: true }
        );
        panel.webview.html = generateGraphHtmlFromRecords(result.records);
      } catch (err: any) {
        vscode.window.showErrorMessage(`Ошибка подключения к Neo4j: ${err.message}`);
      }
    })
  );
}

async function translateDSLToCypher(dslParserPath: string, code: string): Promise<string | undefined> {
  return new Promise<string | undefined>((resolve) => {
    const args = ['-f', '-', '-o', '-', '-t', 'cypher'];
    const child = spawn(dslParserPath, args, { stdio: ['pipe', 'pipe', 'pipe'] });
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
        vscode.window.showErrorMessage(`Ошибка записи в dsl-parser stdin: ${err.message}`);
      }
      child.stdin.end();
    });
  });
}



function generateGraphHtmlFromRecords(records: Record[]): string {
  const nodesMap = new Map<string, any>();

  const isNode = (obj: any): boolean => {
    return obj &&
      typeof obj === 'object' &&
      'identity' in obj &&
      'labels' in obj &&
      'properties' in obj;
  };

  for (const record of records) {
    for (const key of record.keys) {
      const value = record.get(key);
      if (isNode(value)) {
        let nodeId: any = value.identity;
        if (nodeId && typeof nodeId.toInt === 'function') {
          nodeId = nodeId.toInt();
        }
        const nodeKey = String(nodeId);
        if (!nodesMap.has(nodeKey)) {
          nodesMap.set(nodeKey, {
            id: nodeId,
            label: value.properties.name ||
              value.properties['structurizr.dsl.identifier'] ||
              '(без имени)',
            title:
              'ID: ' + (value.properties.id || '') + '\n' +
              'Labels: ' + (value.labels ? value.labels.join(', ') : '') + '\n' +
              'Properties: ' + JSON.stringify(value.properties, null, 2)
          });
        }
      }
    }
  }

  const nodes = Array.from(nodesMap.values());

  const edges: any[] = [];

  return `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Neo4j Graph Visualization</title>
  <style>
    body { margin: 0; padding: 0; font-family: sans-serif; }
    #network { width: 100vw; height: 100vh; border: 1px solid lightgray; }
  </style>
  <!-- Подключаем vis-network через CDN -->
  <script type="text/javascript" src="https://unpkg.com/vis-network/standalone/umd/vis-network.min.js"></script>
</head>
<body>
  <div id="network"></div>
  <script type="text/javascript">
    // Инициализируем узлы и ребра
    const nodes = new vis.DataSet(${JSON.stringify(nodes)});
    const edges = new vis.DataSet(${JSON.stringify(edges)});

    const container = document.getElementById('network');
    const data = { nodes, edges };
    const options = {
      layout: { improvedLayout: true },
      physics: { stabilization: true }
    };

    // Создаем интерактивный граф
    new vis.Network(container, data, options);
  </script>
</body>
</html>`;
}

export function deactivate(): Thenable<void> | undefined {
  return undefined;
}
