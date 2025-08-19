# EPD nRF5 HTML Interface

## 路径说明

### Vercel部署
- 静态文件放在 `public/` 目录下
- HTML中引用时路径自动忽略 `public/` 前缀
- 例如：`public/css/main.css` → `/css/main.css`

### Debug
- 需要手动添加 `public/` 前缀
- 例如：`public/css/main.css` 和 `public/js/main.js`
