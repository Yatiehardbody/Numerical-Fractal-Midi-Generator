// Initial UI settings only; existing compositions and explicit settings remain intact.
(() => {
  const originalDefaults = module_engine.defaults;
  module_engine.defaults = () => ({
    ...module_engine.presetSettings('Fractal', originalDefaults()), engine: 'AUTO'
  });
})();
