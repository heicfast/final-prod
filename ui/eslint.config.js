import eslint from "@eslint/js";
import tseslint from "typescript-eslint";
import reactHooks from "eslint-plugin-react-hooks";
import importx from "eslint-plugin-import-x";
import globals from "globals";

export default tseslint.config(
  {
    ignores: [
      "dist/**",
      "coverage/**",
      "src-tauri/target/**",
      "src-tauri/**",
      "node_modules/**",
      "playwright-report/**",
      "test-results/**",
      "docs/**",
      "worklog/**",
    ],
  },
  eslint.configs.recommended,
  ...tseslint.configs.strictTypeChecked,
  ...tseslint.configs.stylisticTypeChecked,
  {
    files: ["**/*.{ts,tsx}"],
    languageOptions: {
      ecmaVersion: 2022,
      globals: { ...globals.browser, ...globals.es2022 },
      parserOptions: {
        projectService: true,
        tsconfigRootDir: import.meta.dirname,
      },
    },
    plugins: {
      "react-hooks": reactHooks,
      "import-x": importx,
    },
    rules: {
      ...reactHooks.configs.recommended.rules,
      "@typescript-eslint/no-unused-vars": [
        "error",
        { argsIgnorePattern: "^_", varsIgnorePattern: "^_" },
      ],
      "@typescript-eslint/consistent-type-imports": [
        "error",
        { prefer: "type-imports", fixStyle: "inline-type-imports" },
      ],
      "@typescript-eslint/no-misused-promises": [
        "error",
        { checksVoidReturn: { attributes: false } },
      ],
      "@typescript-eslint/no-floating-promises": ["error", { ignoreIIFE: true }],
      // Architecture boundaries — belt and braces alongside ast-grep + scripts/arch-lint.mjs
      "import-x/no-restricted-paths": [
        "error",
        {
          zones: [
            // components/ and app/ may not import features (only their own feature may)
            {
              target: "./src/components",
              from: "./src/features",
            },
            {
              target: "./src/app",
              from: "./src/features",
              except: ["./shell"],
            },
            // cross-feature imports are forbidden
            {
              target: "./src/features/device-selection",
              from: "./src/features/!(device-selection)",
            },
            {
              target: "./src/features/scan-configuration",
              from: "./src/features/!(scan-configuration)",
            },
            {
              target: "./src/features/scan-progress",
              from: "./src/features/!(scan-progress)",
            },
            {
              target: "./src/features/results-explorer",
              from: "./src/features/!(results-explorer)",
            },
            {
              target: "./src/features/preview",
              from: "./src/features/!(preview)",
            },
            {
              target: "./src/features/recovery-queue",
              from: "./src/features/!(recovery-queue)",
            },
            {
              target: "./src/features/health",
              from: "./src/features/!(health)",
            },
            {
              target: "./src/features/repair",
              from: "./src/features/!(repair)",
            },
            {
              target: "./src/features/settings-about",
              from: "./src/features/!(settings-about)",
            },
            {
              target: "./src/features/dev-panel",
              from: "./src/features/!(dev-panel)",
            },
          ],
        },
      ],
      "no-console": ["error", { allow: [] }],
    },
  },
  {
    files: ["tests/**/*.ts", "tests/**/*.tsx", "e2e/**/*.ts", "scripts/**/*.mjs", "vite.config.ts", "playwright.config.ts"],
    rules: {
      "@typescript-eslint/no-unsafe-assignment": "off",
      "@typescript-eslint/no-unsafe-member-access": "off",
      "@typescript-eslint/no-unsafe-call": "off",
      "@typescript-eslint/no-unsafe-argument": "off",
      "@typescript-eslint/no-unsafe-return": "off",
      "@typescript-eslint/only-throw-error": "off",
      "@typescript-eslint/no-floating-promises": "off",
      "no-console": "off",
    },
  },
);
