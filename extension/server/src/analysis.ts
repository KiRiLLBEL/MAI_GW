import {Diagnostic, DiagnosticSeverity} from 'vscode-languageserver';
export async function performSemanticAnalysis(
    ast: any, diagnostics: Diagnostic[],
    scopeStack: Array<Set<string>>): Promise<void> {
  const validFunctions = new Set<string>(
      [ 'route', 'cross', 'union', 'failure_point', 'instance' ]);

  function isDeclared(name: string): boolean {
    for (let i = scopeStack.length - 1; i >= 0; i--) {
      if (scopeStack[i].has(name)) {
        return true;
      }
    }
    return false;
  }

  async function traverse(node: any): Promise<void> {
    if (typeof node !== 'object' || node === null) {
      return;
    }
    const nodeType: string = node.type;

    if (nodeType === 'assignment') {
      if (typeof node.name === 'string') {
        scopeStack[scopeStack.length - 1].add(node.name);
      }
      await traverse(node.expression);
    } else if (nodeType === 'ALL' || nodeType === 'ANY') {
      const newScope = new Set<string>();
      if (Array.isArray(node.args)) {
        for (const id of node.args) {
          if (typeof id === 'string') {
            newScope.add(id);
          }
        }
      }
      scopeStack.push(newScope);
      await traverse(node.source);
      await traverse(node.predicate);
    } else if (nodeType === 'variable') {
      const varName = node['variable'];
      if (typeof varName === 'string' && !isDeclared(varName)) {
        let line = 0, character = 0, length = 1;
        if (node.node && typeof node.node.line === 'number' &&
            typeof node.node.column === 'number') {
          line = node.node.line - 1;
          character = node.node.column - 1;
          if (typeof node.node.length === 'number') {
            length = node.node.length;
          }
        }
        diagnostics.push({
          severity : DiagnosticSeverity.Error,
          range : {
            start : {line, character},
            end : {line, character : character + length}
          },
          message : `Uninitialized variable: ${varName}`,
          source : 'semantic'
        });
      }
    } else if (nodeType === 'call') {
      const funcName = node.name;
      if (typeof funcName === 'string' && !validFunctions.has(funcName)) {
        let line = 0, character = 0, length = 1;
        if (node.node && typeof node.node.line === 'number' &&
            typeof node.node.column === 'number') {
          line = node.node.line - 1;
          character = node.node.column - 1;
          if (typeof node.node.length === 'number') {
            length = node.node.length;
          }
        }
        diagnostics.push({
          severity : DiagnosticSeverity.Error,
          range : {
            start : {line, character},
            end : {line, character : character + length}
          },
          message : `Unknown function: ${funcName}`,
          source : 'semantic'
        });
      }
    }

    if (Array.isArray(node)) {
      for (const elem of node) {
        await traverse(elem);
      }
      return;
    }
    if (!node.hasOwnProperty('type')) {
      await Promise.all(
          Object.keys(node).map(async key => { await traverse(node[key]); }));
      return;
    }

    const childProps = [
      'children', 'blocks', 'statements', 'args', 'statement', 'expression',
      'quantifier', 'cond', 'then', 'else', 'source', 'predicate', 'left',
      'right', 'operand'
    ];
    for (const prop of childProps) {
      if (node.hasOwnProperty(prop)) {
        await traverse(node[prop]);
      }
    }
  }

  await traverse(ast);
}