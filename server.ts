import express from 'express';
import { createServer as createViteServer } from 'vite';
import { exec } from 'child_process';
import path from 'path';
import fs from 'fs';

const app = express();
const PORT = 3000;
const NOVA_OS_DIR = path.resolve(process.cwd(), 'nova-os');

app.use(express.json());

// Helper for running shell commands safely
function runCmd(cmd: string, cwd: string = NOVA_OS_DIR): Promise<{ stdout: string; stderr: string; code: number }> {
  return new Promise((resolve) => {
    exec(cmd, { cwd, timeout: 15000 }, (error, stdout, stderr) => {
      resolve({
        stdout: stdout || '',
        stderr: stderr || '',
        code: error && error.code ? error.code : 0,
      });
    });
  });
}

// GET /api/status: Check toolchain and kernel status
app.get('/api/status', async (_req, res) => {
  const kernelElf = path.join(NOVA_OS_DIR, 'bin', 'nova-kernel.elf');
  const kernelMb = path.join(NOVA_OS_DIR, 'bin', 'nova-kernel-mb.elf');
  const testRunner = path.join(NOVA_OS_DIR, 'bin', 'test_runner');

  let elfSize = 0;
  if (fs.existsSync(kernelElf)) {
    elfSize = fs.statSync(kernelElf).size;
  }

  const gccRes = await runCmd('gcc --version | head -n 1');
  const nasmRes = await runCmd('nasm -v');
  const qemuRes = await runCmd('qemu-system-x86_64 --version | head -n 1');

  res.json({
    system: 'NOVA OS Microkernel Subsystem',
    version: '1.0.0-LTS',
    targetArch: 'x86_64 (Intel 64 / AMD64)',
    bootProtocol: 'Multiboot 1 / UEFI PE32+',
    toolchain: {
      gcc: gccRes.stdout.trim(),
      nasm: nasmRes.stdout.trim(),
      qemu: qemuRes.stdout.trim(),
    },
    artifacts: {
      kernelElfExists: fs.existsSync(kernelElf),
      kernelMbExists: fs.existsSync(kernelMb),
      testRunnerExists: fs.existsSync(testRunner),
      elfSizeBytes: elfSize,
    },
  });
});

// POST /api/build: Compile the kernel and run tests
app.post('/api/build', async (_req, res) => {
  const result = await runCmd('make clean && make all');
  const kernelMb = path.join(NOVA_OS_DIR, 'bin', 'nova-kernel-mb.elf');
  let elfSize = 0;
  if (fs.existsSync(kernelMb)) {
    elfSize = fs.statSync(kernelMb).size;
  }

  // Get ELF headers
  let readelfOutput = '';
  if (fs.existsSync(kernelMb)) {
    const r = await runCmd('readelf -h bin/nova-kernel-mb.elf');
    readelfOutput = r.stdout;
  }

  res.json({
    success: result.code === 0,
    output: result.stdout + (result.stderr ? '\n' + result.stderr : ''),
    elfSizeBytes: elfSize,
    elfHeaders: readelfOutput,
  });
});

// POST /api/test: Run automated test suite and benchmarks
app.post('/api/test', async (_req, res) => {
  const result = await runCmd('./bin/test_runner');
  res.json({
    success: result.code === 0,
    output: result.stdout,
  });
});

// POST /api/qemu-boot: Boot kernel in QEMU and capture serial console
app.post('/api/qemu-boot', async (_req, res) => {
  const logFile = '/tmp/serial_qemu_run.log';
  const kernelPath = path.join(NOVA_OS_DIR, 'bin', 'nova-kernel-mb.elf');

  if (!fs.existsSync(kernelPath)) {
    await runCmd('make kernel');
  }

  const qemuCmd = `rm -f ${logFile} && timeout 2.5s qemu-system-x86_64 -kernel ${kernelPath} -display none -serial file:${logFile} -m 512M || true`;
  await runCmd(qemuCmd);

  let serialLog = '';
  if (fs.existsSync(logFile)) {
    serialLog = fs.readFileSync(logFile, 'utf-8');
  } else {
    serialLog = '[Error] No serial output captured.';
  }

  res.json({
    success: serialLog.includes('NOVA OPERATING SYSTEM'),
    serialLog,
  });
});

// GET /api/source-tree: Get all files
app.get('/api/source-tree', (_req, res) => {
  function getFiles(dir: string, base: string = ''): Array<{ path: string; name: string; size: number }> {
    let results: Array<{ path: string; name: string; size: number }> = [];
    if (!fs.existsSync(dir)) return results;
    const list = fs.readdirSync(dir);
    for (const item of list) {
      if (item === 'build' || item === 'bin' || item === '.git') continue;
      const fullPath = path.join(dir, item);
      const relPath = base ? `${base}/${item}` : item;
      const stat = fs.statSync(fullPath);
      if (stat.isDirectory()) {
        results = results.concat(getFiles(fullPath, relPath));
      } else {
        results.push({ path: relPath, name: item, size: stat.size });
      }
    }
    return results;
  }

  const files = getFiles(NOVA_OS_DIR);
  const rootReadme = path.resolve(process.cwd(), 'README.md');
  if (fs.existsSync(rootReadme)) {
    files.unshift({ path: 'README.md', name: 'README.md', size: fs.statSync(rootReadme).size });
  }

  res.json({ files });
});

// GET /api/source-file: Read specific file
app.get('/api/source-file', (req, res) => {
  const relPath = req.query.path as string;
  if (!relPath || relPath.includes('..')) {
    return res.status(400).send('Invalid file path');
  }
  let fullPath = path.join(NOVA_OS_DIR, relPath);
  if (!fs.existsSync(fullPath)) {
    fullPath = path.resolve(process.cwd(), relPath);
  }
  if (!fs.existsSync(fullPath)) {
    return res.status(404).send('File not found');
  }
  const content = fs.readFileSync(fullPath, 'utf-8');
  res.json({ path: relPath, content });
});

// Boot Vite middleware
async function startServer() {
  const isProd = process.env.NODE_ENV === 'production';
  if (!isProd) {
    const vite = await createViteServer({
      server: { middlewareMode: true },
      appType: 'spa',
    });
    app.use(vite.middlewares);
  } else {
    const distPath = path.resolve(process.cwd(), 'dist');
    app.use(express.static(distPath));
    app.get('*', (_req, res) => {
      res.sendFile(path.resolve(distPath, 'index.html'));
    });
  }

  app.listen(PORT, '0.0.0.0', () => {
    console.log(`NOVA OS System Engine running at http://0.0.0.0:${PORT}`);
  });
}

startServer();
