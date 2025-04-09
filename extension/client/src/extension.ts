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
  // Путь к исполняемому файлу сервера (считаем, что сервер скомпилирован в
  // server/out/server.js)
  const serverModule =
      context.asAbsolutePath(path.join('server', 'out', 'server.js'));
  // Опции запуска: на продакшн и на отладку (Debug)
  const serverOptions: ServerOptions = {
    run : {module : serverModule, transport : TransportKind.ipc},
    debug : {
      module : serverModule,
      transport : TransportKind.ipc,
      options : {execArgv : [ "--nolazy", "--inspect=6009" ]}
    }
  };

  // Опции клиента
  const clientOptions: LanguageClientOptions = {
    documentSelector : [ {scheme : 'file', language : 'arch'} ],
    synchronize :
        {fileEvents : vscode.workspace.createFileSystemWatcher('**/*.arch')}
    // Здесь мы можем указать настройки синхронизации, наблюдение за
    // конфигурацией и файлами, если нужно
  };

  // Создаем и запускаем LanguageClient
  client = new LanguageClient(
      'ArchLanguageServer', // ID клиента (произвольный)
      'Arch Language Server', // Название (для отображения в output, например)
      serverOptions, clientOptions);

  client.start();
}

export function deactivate(): Thenable<void>|undefined {
  return client ? client.stop() : undefined;
}
