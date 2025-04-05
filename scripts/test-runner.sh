#!/usr/bin/env bash
set -e

chmod +x ./artifacts/dsl-parser

docker pull structurizr/cli:latest

echo "=== Starting tests for all examples ==="

for testdir in examples/*; do
  if [ -d "$testdir" ]; then
    echo "===== Running test for: $testdir ====="

    docker run --rm \
      -v "${GITHUB_WORKSPACE}:/usr/local/structurizr" \
      structurizr/cli export \
      -workspace "$testdir/workspace.dsl" \
      -format json \
      -output "$testdir/"

    python converter/converter.py -f "$testdir/workspace.json"

    ./artifacts/dsl-parser \
      -f "$testdir/input.arch" \
      -o "$testdir/test.cypher" \
      -t cypher

    docker exec -i neo4j-test bin/cypher-shell < "$testdir/test.cypher" \
      > "$testdir/actual_result.txt" 2>&1


    diff -u "$testdir/result.txt" "$testdir/actual_result.txt"

    docker exec -i neo4j-test bin/cypher-shell <<EOF
MATCH (n) DETACH DELETE n;
EOF

    echo "===== Test for $testdir PASSED ====="
  fi
done

echo "=== All example tests completed successfully ==="
