// gamecore-zone 文件站 — 輕量互動
(function () {
  // 進場揭示
  const io = new IntersectionObserver((entries) => {
    entries.forEach((e) => { if (e.isIntersecting) { e.target.classList.add('in'); io.unobserve(e.target); } });
  }, { threshold: 0.12 });
  document.querySelectorAll('.reveal').forEach((el, i) => {
    el.style.transitionDelay = (i % 6) * 60 + 'ms';
    io.observe(el);
  });

  // 行動版選單
  const btn = document.querySelector('.menu-btn');
  const nav = document.querySelector('.nav');
  if (btn && nav) btn.addEventListener('click', () => nav.classList.toggle('open'));

  // 點擊指令塊複製
  document.querySelectorAll('pre[data-copy]').forEach((pre) => {
    pre.title = '點擊複製';
    pre.style.cursor = 'copy';
    pre.addEventListener('click', () => {
      const text = pre.innerText.replace(/^\s*\$\s?/gm, '').trim();
      navigator.clipboard?.writeText(text).then(() => {
        const old = pre.getAttribute('data-label') || '';
        pre.setAttribute('data-copied', '已複製 ✓');
        const tag = document.createElement('span');
        tag.textContent = '已複製 ✓';
        tag.style.cssText = 'position:absolute;top:10px;right:14px;font-family:var(--code);font-size:11px;color:var(--heal)';
        pre.appendChild(tag);
        setTimeout(() => tag.remove(), 1200);
      });
    });
  });
})();
