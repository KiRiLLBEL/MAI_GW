const path = require('path');

module.exports = {
  target : 'node',
  mode : 'development',
  entry : {
    extension : './client/src/extension.ts',
    server : './server/src/server.ts'
  },
  output : {
    path : path.resolve(__dirname, 'out'),
    filename : '[name].js',
    libraryTarget : 'commonjs2'
  },
  resolve : {extensions : [ '.ts', '.js' ]},
  externals : {'vscode' : 'commonjs vscode'},
  module : {
    rules : [ {test : /\.ts$/, exclude : /node_modules/, use : 'ts-loader'} ]
  }
};