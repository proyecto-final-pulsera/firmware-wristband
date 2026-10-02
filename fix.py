import re

with open('docs/ARQUITECTURA_TAREAS.md', 'r', encoding='utf-8') as f:
    content = f.read()

# Fix the broken backticks in the entire file
content = content.replace("`mermaid", "```mermaid")
# Fix the end block that might be broken too
content = re.sub(r'StepRepo\n`\n', 'StepRepo\n```\n', content)
# Just to be sure, find the closing backticks properly
lines = content.split('\n')
for i in range(len(lines)):
    if lines[i].strip() == '`':
        lines[i] = '```'

with open('docs/ARQUITECTURA_TAREAS.md', 'w', encoding='utf-8') as f:
    f.write('\n'.join(lines))
