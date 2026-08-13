import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import App from './App.tsx'
import { LocalizationContext } from './LocalizationContext.ts'
import { LocalizationProvider } from './LocalizationProvider.ts'

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <LocalizationContext value={new LocalizationProvider(new Intl.Locale(navigator.language).language)}>
    <App />
    </LocalizationContext>
  </StrictMode>,
)
