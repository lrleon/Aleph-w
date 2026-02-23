# Publicar documentación Doxygen de Aleph-w en GitHub Pages (solo `master`)

Este documento describe el flujo recomendado para **generar y publicar** la documentación Doxygen de Aleph-w en **GitHub Pages**.

> Objetivo: que cada `push` a `master` regenere la documentación y la publique automáticamente en Internet.

---

## 0) Requisitos previos

- El repositorio debe tener el workflow de GitHub Actions:
  - `.github/workflows/docs.yml`
- En el repo existe un `Doxyfile` configurado para generar HTML.
  - En este repo, el `Doxyfile` genera en: `docs/html/`.

---

## 1) Configurar GitHub Pages (una sola vez)

1. Abre el repositorio en GitHub:
   - `https://github.com/<owner>/<repo>`

2. Ve a:
   - **Settings → Pages**

3. En **Build and deployment** busca **Source** y selecciona:
   - **GitHub Actions**

   Si en tu UI no aparece **GitHub Actions** todavía:
   - Ejecuta el workflow **Docs (Doxygen)** al menos una vez (ver sección 2)
   - Recarga la página **Settings → Pages**

---

## 2) Generar la documentación (GitHub Actions)

Tienes dos formas:

### Opción A: Automática (push a `master`)

Cada vez que hagas `push` a `master`, el workflow `docs.yml` debe ejecutarse automáticamente.

### Opción B: Manual (workflow_dispatch)

1. Ve a la pestaña **Actions**
2. Selecciona el workflow **Docs (Doxygen)**
3. Click en **Run workflow**
4. Selecciona `master`
5. Confirma con **Run workflow**

---

## 3) Verificar que se publicó correctamente

1. Ve a **Actions**
2. Abre la corrida del workflow **Docs (Doxygen)**
3. Entra al job:
   - `build-and-deploy (GitHub Pages)`

Verifica que los pasos terminen en verde:
- Checkout
- Install dependencies
- Build Doxygen HTML
- Configure Pages
- Upload Pages artifact
- Deploy to GitHub Pages

### Encontrar el URL publicado

Cuando la corrida termina:
- En la página del workflow debe aparecer un enlace de deployment (environment `github-pages`), o
- La URL típica será:
  - `https://<owner>.github.io/<repo>/`

En este repo, típicamente:
- `https://lrleon.github.io/Aleph-w/`

---

## 4) Qué se publica exactamente

- El workflow ejecuta:
  - `doxygen Doxyfile`
- Luego publica el HTML generado en:
  - `docs/html/`

Esto significa:
- No hace falta commitear el HTML generado
- GitHub Pages sirve directamente el contenido generado por CI

---

## 5) Problemas comunes (troubleshooting)

### 5.1 No veo “GitHub Actions” en Settings → Pages
- Ejecuta **Docs (Doxygen)** una vez desde **Actions**
- Recarga la página **Settings → Pages**

### 5.2 Sale 404 en la URL
- Confirma que el workflow terminó **exitosamente**
- Espera 1–2 minutos y recarga

### 5.3 Fallo en “Deploy to GitHub Pages”
Revisar configuración:
- **Settings → Actions → General → Workflow permissions**
  - Recomendado: **Read and write permissions**

### 5.4 Repo privado
Dependiendo del plan/configuración de GitHub:
- GitHub Pages puede estar limitado para repos privados

---

## 6) Flujo operativo recomendado

- Desarrollo normal en branches
- Merge a `master`
- Cada push a `master` publica la documentación actualizada automáticamente

---

## Checklist rápido

- [ ] Existe `.github/workflows/docs.yml`
- [ ] Settings → Pages → Source: **GitHub Actions**
- [ ] Workflow **Docs (Doxygen)** corre en verde
- [ ] URL de Pages abre correctamente
